#define _POSIX_C_SOURCE 200809L

#include "blake3.h"
#include "common.h"
#include "door.h"
#include "json_util.h"
#include "mcp.h"
#include "server.h"
#include "store.h"

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

static void rm_rf(const char *path) {
  struct stat st;
  if (lstat(path, &st) != 0) return;
  if (S_ISDIR(st.st_mode)) {
    DIR *d = opendir(path);
    if (d) {
      struct dirent *ent;
      while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char sub[512];
        snprintf(sub, sizeof(sub), "%s/%s", path, ent->d_name);
        rm_rf(sub);
      }
      closedir(d);
    }
    rmdir(path);
  } else {
    unlink(path);
  }
}

static long read_vm_rss_kb(void) {
  FILE *fp = fopen("/proc/self/status", "r");
  if (!fp) return -1;
  char line[256];
  long rss = -1;
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      rss = strtol(line + 6, NULL, 10);
      break;
    }
  }
  fclose(fp);
  return rss;
}

/* JSON-escape a string into a malloc'd buffer (without surrounding quotes) */
static char *json_escape_str(const char *s, size_t len) {
  size_t cap = len * 2U + 16U;
  char *out = malloc(cap);
  if (!out) return NULL;
  size_t pos = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)s[i];
    if (pos + 8U > cap) {
      cap *= 2U;
      char *nb = realloc(out, cap);
      if (!nb) { free(out); return NULL; }
      out = nb;
    }
    if (c == '"') { out[pos++] = '\\'; out[pos++] = '"'; }
    else if (c == '\\') { out[pos++] = '\\'; out[pos++] = '\\'; }
    else if (c == '\n') { out[pos++] = '\\'; out[pos++] = 'n'; }
    else if (c == '\r') { out[pos++] = '\\'; out[pos++] = 'r'; }
    else if (c == '\t') { out[pos++] = '\\'; out[pos++] = 't'; }
    else { out[pos++] = (char)c; }
  }
  out[pos] = '\0';
  return out;
}

static bool extract_mcp_text(const char *json, size_t jlen, char **out, size_t *out_len) {
  JsonSpan res = {NULL, 0};
  if (stigi_json_obj_get_object(json, jlen, "result", &res) != JSON_OK) return false;
  /* Find "text":"..." inside content[0] or contents[0] */
  const char *p = strstr(res.ptr, "{\"type\":\"text\",\"text\":");
  if (!p) p = strstr(res.ptr, "\"text\":");
  if (!p) return false;
  const char *brace = p;
  while (brace > res.ptr && *brace != '{') brace--;
  if (*brace != '{') return false;
  size_t rem = (size_t)((res.ptr + res.len) - brace);
  return stigi_json_obj_get_string(brace, rem, "text", out, out_len) == JSON_OK;
}

typedef struct {
  StigiServerCtx *ctx;
  const char *sock_path;
  bool cc_socks;
  uint32_t max_reqs;
  Status status;
  Diag diag;
} SockThreadArgs;

static void *sock_server_thread(void *arg) {
  SockThreadArgs *a = (SockThreadArgs *)arg;
  if (a->cc_socks) {
    a->status = stigi_serve_cc_socks(a->ctx, a->sock_path, a->max_reqs, &a->diag);
  } else {
    a->status = stigi_serve_socket(a->ctx, a->sock_path, a->max_reqs, &a->diag);
  }
  return NULL;
}

static int wait_for_sock(const char *path) {
  struct stat st;
  for (int tries = 0; tries < 200; tries++) {
    if (stat(path, &st) == 0) return 0;
    struct timespec ts = {0, 5000000L}; /* 5 ms */
    nanosleep(&ts, NULL);
  }
  return -1;
}

static Status send_cc_socks_line(const char *sock_path, const char *line, size_t len,
                                 char **out_reply, size_t *out_len, Diag *diag) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return diag_set(diag, STIGI_ERR_IO, -1, "socket() failed");
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sock_path);
  if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
    close(fd);
    return diag_set(diag, STIGI_ERR_IO, -1, "connect() failed");
  }
  /* Write newline-delimited JSON WITHOUT shutting down write half (tests no EOF deadlock!) */
  size_t off = 0;
  while (off < len) {
    ssize_t n = write(fd, line + off, len - off);
    if (n <= 0) { close(fd); return diag_set(diag, STIGI_ERR_IO, -1, "write() failed"); }
    off += (size_t)n;
  }
  size_t cap = 4096U, rlen = 0;
  char *buf = malloc(cap);
  if (!buf) { close(fd); return diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); }
  for (;;) {
    if (rlen + 4096U + 1U > cap) {
      cap *= 2U;
      char *nb = realloc(buf, cap);
      if (!nb) { free(buf); close(fd); return diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); }
      buf = nb;
    }
    ssize_t n = read(fd, buf + rlen, 4096U);
    if (n <= 0) break;
    rlen += (size_t)n;
  }
  buf[rlen] = '\0';
  close(fd);
  *out_reply = buf;
  if (out_len) *out_len = rlen;
  return STIGI_OK;
}

static int test_store_load_roundtrip(void) {
  const char *store_dir = "/tmp/stigi-c9-store";
  rm_rf(store_dir);

  StigiDoor door;
  Diag diag = {0};
  CHECK(stigi_door_init(&door, "vic/spec/datalog/all.dl",
                        STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK,
        "door init");

  /* S132: the seed id is the BLAKE3 of this exact binary (independent path: one full read). */
  {
    FILE *exe = fopen("/proc/self/exe", "rb");
    struct stat exe_stat;
    CHECK(exe != NULL && fstat(fileno(exe), &exe_stat) == 0 && exe_stat.st_size > 0, "stat /proc/self/exe");
    size_t exe_len = (size_t)exe_stat.st_size;
    uint8_t *exe_bytes = malloc(exe_len);
    CHECK(exe_bytes != NULL && fread(exe_bytes, 1, exe_len, exe) == exe_len, "read /proc/self/exe");
    (void)fclose(exe);
    uint8_t want[32];
    blake3_hash(exe_bytes, exe_len, want);
    free(exe_bytes);
    CHECK(memcmp(door.seed_id, want, 32) == 0, "S132: seed_id == BLAKE3 of the running binary");
  }

  const char *u1 =
      "// prompt: prove zero is non-negative\n"
      "pub lemma def Demo.zeroBound(value: U64) <lessOrEqual(0, value)>: <lessOrEqual(0, value)> =\n"
      "  @<lessOrEqual(0, value)>\n";
  const char *u2 =
      "// prompt: compute double of value\n"
      "pub def Demo.doubleVal(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 1000)>: U64 = value + value\n";
  const char *u3 =
      "// prompt: main entry calling doubleVal\n"
      "pub def main(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 1000)>: U64 =\n"
      "  Demo.zeroBound(value)\n"
      "  Demo.doubleVal(value)\n";
  const char *u4 =
      "// prompt: test doubleVal works\n"
      "val Demo.Tests.checkDouble {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(Demo.doubleVal(21), 42, \"21 + 21 == 42\")\n";
  const char *srcs[4] = {u1, u2, u3, u4};
  size_t lens[4] = {strlen(u1), strlen(u2), strlen(u3), strlen(u4)};
  StigiDoorResult res;
  stigi_door_result_init(&res);
  CHECK(stigi_door_submit(&door, srcs, lens, 4, &res, &diag) == STIGI_OK, "submit");
  CHECK(res.accepted, "initial submit accepted");
  stigi_door_result_free(&res);

  uint32_t orig_count = door.image.unit_count;
  CHECK(orig_count == 4, "4 units in image");
  char orig_names[4][128];
  uint8_t orig_ids[4][32];
  for (uint32_t i = 0; i < orig_count; i++) {
    snprintf(orig_names[i], sizeof(orig_names[i]), "%s", door.image.units[i].name);
    memcpy(orig_ids[i], door.image.units[i].unit_id, 32);
  }

  /* Negative control: missing store dir MUST be rejected with STIGI_ERR_IO (door.md:124) */
  char err[256] = {0};
  CHECK(stigi_store_dir(&door, store_dir, err, sizeof(err)) == STIGI_ERR_IO,
        "store to missing dir rejected with STIGI_ERR_IO");

  /* Create store_dir first, then store */
  CHECK(mkdir(store_dir, 0755) == 0, "mkdir store_dir");
  CHECK(stigi_store_dir(&door, store_dir, err, sizeof(err)) == STIGI_OK, "store_dir ok");

  struct stat st;
  CHECK(stat("/tmp/stigi-c9-store/index", &st) == 0 && st.st_size > 0, "index file created");
  CHECK(stat("/tmp/stigi-c9-store/cache", &st) == 0 && st.st_size > 0, "cache file created");

  /* Defect #26: Load into a BRAND-NEW FRESH StigiDoor -> 100% cache hit (0 recomputed!) */
  StigiDoor fresh_door;
  CHECK(stigi_door_init(&fresh_door, "vic/spec/datalog/all.dl",
                        STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK,
        "fresh door init");
  stigi_door_result_init(&res);
  CHECK(stigi_load_dir(&fresh_door, store_dir, &res, &diag) == STIGI_OK, "fresh load_dir ok");
  CHECK(res.accepted, "fresh load_dir accepted");
  CHECK(res.verdicts_computed == 0 && res.tests_computed == 0,
        "fresh door reload hits persisted cache: 0 verdicts/tests recomputed");
  CHECK(res.verdicts_cached == orig_count && res.tests_cached == 1,
        "all verdicts and tests cached on fresh door reload");
  stigi_door_result_free(&res);

  CHECK(fresh_door.image.unit_count == orig_count, "unit count preserved after fresh load");
  for (uint32_t i = 0; i < orig_count; i++) {
    int32_t idx = stigi_image_find(&fresh_door.image, orig_names[i], strlen(orig_names[i]));
    CHECK(idx >= 0, "unit found after fresh reload");
    CHECK(memcmp(fresh_door.image.units[idx].unit_id, orig_ids[i], 32) == 0,
          "runtime_id identical after fresh reload");
  }

  /* Modify 1 root .sc file on disk (main.sc) and reload into fresh door -> verdicts_computed == 1 */
  FILE *fp = fopen("/tmp/stigi-c9-store/main.sc", "wb");
  CHECK(fp != NULL, "open main.sc");
  const char *u3_mod =
      "// prompt: main entry calling doubleVal updated\n"
      "pub def main(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 1000)>: U64 =\n"
      "  Demo.zeroBound(value)\n"
      "  Demo.doubleVal(value) + 0\n";
  fwrite(u3_mod, 1, strlen(u3_mod), fp);
  fclose(fp);

  StigiDoor fresh_door2;
  CHECK(stigi_door_init(&fresh_door2, "vic/spec/datalog/all.dl",
                        STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK,
        "fresh door2 init");
  stigi_door_result_init(&res);
  CHECK(stigi_load_dir(&fresh_door2, store_dir, &res, &diag) == STIGI_OK, "load_dir modified ok");
  CHECK(res.accepted, "load_dir modified accepted");
  CHECK(res.verdicts_computed == 1, "exactly 1 edited root unit recomputed");
  CHECK(res.verdicts_cached == 3 && res.tests_cached == 1,
        "3 unchanged units and 1 test stayed cached");
  stigi_door_result_free(&res);

  /* Negative control: corrupt cache file MUST be rejected AND leave existing cache intact (T0) */
  uint32_t cache_before = fresh_door2.verdict_cache.count;
  FILE *cf = fopen("/tmp/stigi-c9-store/cache", "r+b");
  CHECK(cf != NULL, "open cache for corruption test");
  CHECK(fseek(cf, 20, SEEK_SET) == 0, "seek cache");
  fputc(0xFF, cf);
  fclose(cf);
  stigi_door_result_init(&res);
  CHECK(stigi_load_dir(&fresh_door2, store_dir, &res, &diag) == STIGI_ERR_INVALID,
        "corrupt cache file rejected");
  CHECK(fresh_door2.verdict_cache.count == cache_before,
        "T0: rejected load_dir leaves existing verdict cache intact");
  stigi_door_result_free(&res);

  /* Subset store: store DIR NAME writes only the requested unit */
  const char *sub_dir = "/tmp/stigi-c9-store-sub";
  rm_rf(sub_dir);
  CHECK(mkdir(sub_dir, 0755) == 0, "mkdir sub_dir");
  const char *sub_names[1] = {"Demo.doubleVal"};
  CHECK(stigi_store_dir_subset(&door, sub_dir, sub_names, 1, err, sizeof(err)) == STIGI_OK,
        "store_dir_subset single unit ok");
  struct stat sst;
  CHECK(stat("/tmp/stigi-c9-store-sub/Demo/doubleVal.sc", &sst) == 0,
        "subset store wrote Demo/doubleVal.sc");
  CHECK(stat("/tmp/stigi-c9-store-sub/main.sc", &sst) != 0,
        "subset store omitted non-selected main.sc");
  rm_rf(sub_dir);

  /* Restore clean store, then corrupt index file -> MUST be rejected */
  CHECK(stigi_store_dir(&door, store_dir, err, sizeof(err)) == STIGI_OK, "restore store");
  FILE *ixf = fopen("/tmp/stigi-c9-store/index", "wb");
  CHECK(ixf != NULL, "open index for corruption test");
  fputs("Demo.bad\tNOT_A_VALID_HEX_ID\tDef\n", ixf);
  fclose(ixf);
  stigi_door_result_init(&res);
  CHECK(stigi_load_dir(&fresh_door2, store_dir, &res, &diag) == STIGI_ERR_INVALID,
        "corrupt index file rejected");
  stigi_door_result_free(&res);

  stigi_door_free(&fresh_door2);
  stigi_door_free(&fresh_door);
  stigi_door_free(&door);
  rm_rf(store_dir);
  printf("c9 store/load round-trip: PASS (fresh_cache_hit=0_recomputed, 1_edit=1_recomputed, corrupt_rejected)\n");
  return 0;
}

/* frames.md 7.2: a --cc-socks reply names this peer and its permission class; the registry
   entry carries the fields a Claude Code session lists peers by. */
static int test_cc_peer_identity(void) {
  Diag diag = {0};
  StigiServerCtx ctx;
  stigi_server_init(&ctx, NULL);
  char *frame = NULL;
  size_t flen = 0;
  CHECK(stigi_cc_peer_reply_frame(&ctx, "/tmp/me.sock", "ok\n", 3, &frame, &flen, &diag) == STIGI_OK,
        "default reply frame");
  CHECK(strstr(frame, "from-name=\\\"stigi\\\" from-mode=\\\"prompting\\\"") != NULL,
        "default reply is from stigi, prompting");
  free(frame);
  ctx.peer_name = "tree-peer";
  ctx.from_mode = STIGI_CC_FROM_BYPASS;
  CHECK(stigi_cc_peer_reply_frame(&ctx, "/tmp/me.sock", "ok\n", 3, &frame, &flen, &diag) == STIGI_OK,
        "configured reply frame");
  CHECK(strstr(frame, "from-name=\\\"tree-peer\\\" from-mode=\\\"bypass\\\"") != NULL,
        "reply carries the --peer name and --from-mode");
  free(frame);
  stigi_server_free(&ctx);
  CHECK(stigi_cc_peer_name_valid("stigi-tree") && !stigi_cc_peer_name_valid("a\"b") &&
            !stigi_cc_peer_name_valid("a<b") && !stigi_cc_peer_name_valid(""),
        "peer names that break the wrapper are refused");

  /* Registry: pidDomain and version come from a live peer entry (the parent process). */
  const char *home = "/tmp/stigi-c9-home";
  rm_rf(home);
  CHECK(mkdir(home, 0700) == 0 && mkdir("/tmp/stigi-c9-home/.claude", 0700) == 0 &&
            mkdir("/tmp/stigi-c9-home/.claude/sessions", 0700) == 0,
        "fake HOME");
  char live[256];
  (void)snprintf(live, sizeof live, "%s/.claude/sessions/%ld.json", home, (long)getppid());
  FILE *lf = fopen(live, "wb");
  CHECK(lf != NULL, "fake live peer entry");
  fprintf(lf, "{\"pid\":%ld,\"version\":\"9.9.9\",\"pidDomain\":\"test-domain\"}", (long)getppid());
  (void)fclose(lf);
  char *old_home = getenv("HOME") != NULL ? strdup(getenv("HOME")) : NULL;
  CHECK(setenv("HOME", home, 1) == 0, "set HOME");
  char reg[512];
  Status rs = stigi_cc_peer_register("tree-peer", "/tmp/tree.sock", reg, sizeof reg, &diag);
  if (old_home != NULL) (void)setenv("HOME", old_home, 1);
  free(old_home);
  CHECK(rs == STIGI_OK, "register");
  char body[4096];
  FILE *rf = fopen(reg, "rb");
  CHECK(rf != NULL, "read registration");
  size_t bn = fread(body, 1, sizeof body - 1U, rf);
  body[bn] = '\0';
  (void)fclose(rf);
  char *sid = strstr(body, "\"sessionId\":\"");
  CHECK(sid != NULL && strlen(sid) > 49 && sid[13 + 36] == '"' && sid[13 + 8] == '-',
        "sessionId is a UUID");
  CHECK(strstr(body, "\"pidDomain\":\"test-domain\"") != NULL && strstr(body, "\"version\":\"9.9.9\"") != NULL,
        "pidDomain and version copied from the live peer");
  CHECK(strstr(body, "\"startedAt\":") != NULL && strstr(body, "\"cwd\":\"/") != NULL &&
            strstr(body, "\"name\":\"tree-peer\"") != NULL,
        "startedAt, cwd and name present");
  rm_rf(home);
  printf("c9 cc-socks peer identity: PASS\n");
  return 0;
}

static int test_3transport_parity_and_negatives(void) {
  const char *core_sock = "/tmp/stigi-c9-core.sock";
  const char *cc_sock = "/tmp/stigi-c9-cc.sock";
  unlink(core_sock);
  unlink(cc_sock);

  /* 3 completely independent server instances (Defect #27) */
  StigiDoor door_sock, door_cc, door_mcp;
  Diag diag = {0};
  CHECK(stigi_door_init(&door_sock, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK, "init sock door");
  CHECK(stigi_door_init(&door_cc, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK, "init cc door");
  CHECK(stigi_door_init(&door_mcp, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK, "init mcp door");

  StigiServerCtx srv_sock, srv_cc, srv_mcp;
  stigi_server_init(&srv_sock, &door_sock);
  stigi_server_init(&srv_cc, &door_cc);
  stigi_server_init(&srv_mcp, &door_mcp);

  /* Mutating + read frame sequence across all 3 transports */
  const char *frames[6] = {
      /* 0: batch with 1 good component + 1 bad component (index-unproved with '<' in prompt -> checked fix1!) */
      "batch\n"
      "new // prompt: passing arithmetic test\n"
      "val Demo.Tests.passTwo {Stigi.Test}: Stigi.Test.Result = Stigi.Test.expect(2 + 2, 4, \"two plus two\")\n"
      "new // prompt: read element at index < count\n"
      "pub def main(count: U64, items: Array[U64](count), index: U64) <lessOrEqual(0, count)>: U64 = items(index)\n",
      /* 1: accept fix1 -> admits repaired main (from last reply's checked fix1) */
      "accept fix1\n",
      /* 2: non-empty check frame */
      "check\n"
      "new // prompt: dry-run test\n"
      "val Demo.Tests.dryCheck {Stigi.Test}: Stigi.Test.Result = Stigi.Test.expect(3 + 4, 7, \"three plus four\")\n",
      /* 3: summary */
      "summary\n",
      /* 4: laws */
      "laws\n",
      /* 5: tests */
      "tests\n",
  };

  SockThreadArgs t_sock = {&srv_sock, core_sock, false, 6, STIGI_OK, {0}};
  SockThreadArgs t_cc = {&srv_cc, cc_sock, true, 6, STIGI_OK, {0}};
  pthread_t tid_sock, tid_cc;
  CHECK(pthread_create(&tid_sock, NULL, sock_server_thread, &t_sock) == 0, "spawn core sock thread");
  CHECK(pthread_create(&tid_cc, NULL, sock_server_thread, &t_cc) == 0, "spawn cc-socks thread");
  CHECK(wait_for_sock(core_sock) == 0, "core socket appeared");
  CHECK(wait_for_sock(cc_sock) == 0, "cc socket appeared");

  struct stat st;
  CHECK(stat(core_sock, &st) == 0 && (st.st_mode & 0777) == 0600, "core socket permissions 0600");
  CHECK(stat(cc_sock, &st) == 0 && (st.st_mode & 0777) == 0600, "cc socket permissions 0600");

  for (int i = 0; i < 6; i++) {
    /* 1. Core Unix socket */
    char *r_sock = NULL;
    size_t l_sock = 0;
    CHECK(stigi_send_socket_frame(core_sock, frames[i], strlen(frames[i]),
                                  &r_sock, &l_sock, &diag) == STIGI_OK,
          "core socket frame ok");

    /* 2. --cc-socks newline-delimited JSON envelope over Unix socket */
    char *esc = json_escape_str(frames[i], strlen(frames[i]));
    CHECK(esc != NULL, "escape frame");
    size_t env_cap = strlen(esc) + 128U;
    char *env = malloc(env_cap);
    CHECK(env != NULL, "alloc env");
    snprintf(env, env_cap,
             "{\"from\":\"claude-peer\",\"body\":\"<cross-session-message>%s</cross-session-message>\"}\n",
             esc);
    char *r_cc = NULL;
    size_t l_cc = 0;
    CHECK(send_cc_socks_line(cc_sock, env, strlen(env), &r_cc, &l_cc, &diag) == STIGI_OK,
          "cc-socks frame ok");
    free(env);

    /* 3. MCP tools/call stigi_frame */
    size_t mreq_cap = strlen(esc) + 192U;
    char *mreq = malloc(mreq_cap);
    CHECK(mreq != NULL, "alloc mreq");
    snprintf(mreq, mreq_cap,
             "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"tools/call\","
             "\"params\":{\"name\":\"stigi_frame\",\"arguments\":{\"frame\":\"%s\"}}}",
             i + 1, esc);
    free(esc);
    char *mjson = NULL;
    size_t mjlen = 0;
    CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, mreq, strlen(mreq), &mjson, &mjlen, &diag) == STIGI_OK,
          "mcp tools/call ok");
    free(mreq);
    char *r_mcp = NULL;
    size_t l_mcp = 0;
    CHECK(extract_mcp_text(mjson, mjlen, &r_mcp, &l_mcp), "extract mcp text");
    free(mjson);

    /* Verify byte-identical replies across ALL 3 transports */
    CHECK(l_cc == l_sock && memcmp(r_cc, r_sock, l_sock) == 0,
          "cc-socks reply byte-identical to core socket");
    CHECK(l_mcp == l_sock && memcmp(r_mcp, r_sock, l_sock) == 0,
          "mcp reply byte-identical to core socket");

    if (i == 0) {
      CHECK(strstr(r_sock, "admitted component 1") != NULL, "batch admitted component 1");
      CHECK(strstr(r_sock, "rejected: index-unproved") != NULL, "batch rejected index-unproved");
      CHECK(strstr(r_sock, "fix1 (checked)") != NULL, "batch offered fix1 (checked)");
    } else if (i == 1) {
      CHECK(strstr(r_sock, "admitted main") != NULL, "accept fix1 admitted repaired main");
    } else if (i == 2) {
      CHECK(strstr(r_sock, "admitted component 1") != NULL, "non-empty check admitted");
    } else if (i == 5) {
      CHECK(strncmp(r_sock, "accepted tests ", 15) == 0, "tests reply starts with accepted tests");
    }

    free(r_sock);
    free(r_cc);
    free(r_mcp);
  }

  pthread_join(tid_sock, NULL);
  pthread_join(tid_cc, NULL);
  CHECK(t_sock.status == STIGI_OK, "core socket server exited cleanly");
  CHECK(t_cc.status == STIGI_OK, "cc-socks server exited cleanly");

  /* Verify identical image root IDs across all 3 independent doors */
  CHECK(door_sock.image.unit_count == 2 &&
        door_cc.image.unit_count == 2 &&
        door_mcp.image.unit_count == 2,
        "all 3 independent doors have 2 admitted units");
  for (uint32_t i = 0; i < door_sock.image.unit_count; i++) {
    const char *nm = door_sock.image.units[i].name;
    int32_t icc = stigi_image_find(&door_cc.image, nm, strlen(nm));
    int32_t imcp = stigi_image_find(&door_mcp.image, nm, strlen(nm));
    CHECK(icc >= 0 && imcp >= 0, "unit present in all 3 doors");
    CHECK(memcmp(door_sock.image.units[i].unit_id, door_cc.image.units[icc].unit_id, 32) == 0,
          "unit_id identical between core socket and cc-socks");
    CHECK(memcmp(door_sock.image.units[i].unit_id, door_mcp.image.units[imcp].unit_id, 32) == 0,
          "unit_id identical between core socket and mcp");
  }

  /* ---- Defect #28: Negative controls ---- */
  char *mcp_out = NULL;
  size_t mcp_olen = 0;

  /* 1. JSON-RPC notification (no "id") -> NULL reply, 0 bytes */
  const char *notif = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
  CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, notif, strlen(notif), &mcp_out, &mcp_olen, &diag) == STIGI_OK,
        "notification ok");
  CHECK(mcp_out == NULL && mcp_olen == 0, "notification produces 0-byte NULL reply");

  /* 2. Malformed / unterminated JSON -> -32700 */
  const char *bad_json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"ping";
  CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, bad_json, strlen(bad_json), &mcp_out, &mcp_olen, &diag) == STIGI_OK,
        "malformed json handled");
  CHECK(mcp_out != NULL && strstr(mcp_out, "-32700") != NULL, "unterminated JSON returns -32700");
  free(mcp_out);

  /* 3. Wrong MCP tool name -> -32602 */
  const char *wrong_tool =
      "{\"jsonrpc\":\"2.0\",\"id\":\"req-long-string-id-0123456789\",\"method\":\"tools/call\","
      "\"params\":{\"name\":\"not_stigi_frame\",\"arguments\":{\"frame\":\"summary\\n\"}}}";
  CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, wrong_tool, strlen(wrong_tool), &mcp_out, &mcp_olen, &diag) == STIGI_OK,
        "wrong tool handled");
  CHECK(mcp_out != NULL && strstr(mcp_out, "-32602") != NULL, "wrong MCP tool name returns -32602");
  CHECK(strstr(mcp_out, "req-long-string-id-0123456789") != NULL, "arbitrary string id echoed");
  free(mcp_out);

  /* 4. ping -> {} and resources/list includes stigi://unit/main */
  const char *ping_req = "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"ping\"}";
  CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, ping_req, strlen(ping_req), &mcp_out, &mcp_olen, &diag) == STIGI_OK, "ping ok");
  CHECK(strstr(mcp_out, "\"result\":{}") != NULL, "ping returns {}");
  free(mcp_out);

  const char *rlist_req = "{\"jsonrpc\":\"2.0\",\"id\":100,\"method\":\"resources/list\"}";
  CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, rlist_req, strlen(rlist_req), &mcp_out, &mcp_olen, &diag) == STIGI_OK, "resources/list ok");
  CHECK(strstr(mcp_out, "stigi://unit/main") != NULL, "resources/list exposes stigi://unit/main");
  free(mcp_out);

  /* 5. Stale fix1 after submit MUST be rejected */
  const char *bad_batch =
      "batch\n"
      "new // prompt: bad index read\n"
      "pub def main(count: U64, items: Array[U64](count), index: U64) <lessOrEqual(0, count)>: U64 = items(index)\n";
  char *rep = NULL;
  size_t rlen = 0;
  CHECK(stigi_handle_frame(&srv_mcp, bad_batch, strlen(bad_batch), &rep, &rlen, &diag) == STIGI_OK, "bad batch");
  CHECK(strstr(rep, "fix1 (checked)") != NULL, "fix1 generated");
  free(rep);

  const char *sub_other =
      "submit\n"
      "// prompt: unrelated submit\n"
      "val Demo.Tests.other {Stigi.Test}: Stigi.Test.Result = Stigi.Test.expect(1, 1, \"one\")\n";
  CHECK(stigi_handle_frame(&srv_mcp, sub_other, strlen(sub_other), &rep, &rlen, &diag) == STIGI_OK, "submit other");
  CHECK(strstr(rep, "accepted submit") != NULL, "submit other accepted");
  free(rep);

  CHECK(stigi_handle_frame(&srv_mcp, "accept fix1\n", 12, &rep, &rlen, &diag) == STIGI_OK, "accept stale fix1");
  CHECK(strstr(rep, "error unknown fix 'fix1'") != NULL, "stale fix1 rejected after submit");
  free(rep);

  /* 6. Mutually dependent units (increment + caller Test) in ONE batch admitted atomically! */
  const char *dep_batch =
      "batch\n"
      "new // prompt: test calling increment\n"
      "val Demo.Tests.checkIncrement {Stigi.Test}: Stigi.Test.Result = Stigi.Test.expect(Demo.increment(10), 11, \"10 + 1 == 11\")\n"
      "new // prompt: increment value by one\n"
      "def Demo.increment(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 1\n";
  CHECK(stigi_handle_frame(&srv_mcp, dep_batch, strlen(dep_batch), &rep, &rlen, &diag) == STIGI_OK, "dep batch ok");
  CHECK(strstr(rep, "admitted component 1") != NULL, "increment + caller admitted atomically in 1 component");
  CHECK(strstr(rep, "rejected") == NULL, "no rejection on dependent batch");
  free(rep);

  /* 7. drop breaking dependents rejected in both `check` and `drop` */
  const char *chk_drop = "check\ndrop Demo.increment\n";
  CHECK(stigi_handle_frame(&srv_mcp, chk_drop, strlen(chk_drop), &rep, &rlen, &diag) == STIGI_OK, "check drop");
  CHECK(strstr(rep, "rejected") != NULL, "check drop breaking dependent rejected");
  free(rep);

  CHECK(stigi_handle_frame(&srv_mcp, "drop Demo.increment\n", 20, &rep, &rlen, &diag) == STIGI_OK, "drop increment");
  CHECK(strstr(rep, "rejected drop") != NULL, "drop breaking dependent rejected");
  free(rep);

  /* 8. S119: proof-fact-only edit on callee preserves unit_id on BOTH callee & caller,
   *    while changing verdict_key on BOTH callee & caller! */
  int32_t hi = stigi_image_find(&door_mcp.image, "Demo.increment", 14);
  int32_t ci = stigi_image_find(&door_mcp.image, "Demo.Tests.checkIncrement", 25);
  CHECK(hi >= 0 && ci >= 0, "increment and caller in image");
  uint8_t h_uid_before[32], h_vk_before[32], c_uid_before[32], c_vk_before[32];
  memcpy(h_uid_before, door_mcp.image.units[hi].unit_id, 32);
  memcpy(h_vk_before, door_mcp.image.units[hi].verdict_key, 32);
  memcpy(c_uid_before, door_mcp.image.units[ci].unit_id, 32);
  memcpy(c_vk_before, door_mcp.image.units[ci].verdict_key, 32);

  const char *fact_edit =
      "submit\n"
      "// prompt: increment value by one with extra proof fact\n"
      "def Demo.increment(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 <lessOrEqual(0, value)> = value + 1\n";
  CHECK(stigi_handle_frame(&srv_mcp, fact_edit, strlen(fact_edit), &rep, &rlen, &diag) == STIGI_OK, "fact edit");
  CHECK(strstr(rep, "accepted submit") != NULL, "proof-fact-only edit accepted");
  free(rep);

  hi = stigi_image_find(&door_mcp.image, "Demo.increment", 14);
  ci = stigi_image_find(&door_mcp.image, "Demo.Tests.checkIncrement", 25);
  CHECK(hi >= 0 && ci >= 0, "increment and caller still in image");
  CHECK(memcmp(h_uid_before, door_mcp.image.units[hi].unit_id, 32) == 0,
        "S119: callee runtime unit_id unchanged after proof-fact-only edit");
  CHECK(memcmp(c_uid_before, door_mcp.image.units[ci].unit_id, 32) == 0,
        "S119: caller runtime unit_id unchanged after callee proof-fact-only edit");
  CHECK(memcmp(h_vk_before, door_mcp.image.units[hi].verdict_key, 32) != 0,
        "S119: callee verdict_key invalidated after proof-fact-only edit");
  CHECK(memcmp(c_vk_before, door_mcp.image.units[ci].verdict_key, 32) != 0,
        "S119: caller verdict_key invalidated after callee proof-fact-only edit");

  /* 8b. S136: a type with facts is nominal: two such types with equal bodies are two units with two ids
   *     (the qualified name is in both hashes); the defs that take them differ too */
  const char *nominal =
      "batch\n"
      "new // prompt: first nominal capacity.\n"
      "type Demo.Nominal.FirstCap = U64 <below(self, 100)>\n"
      "new // prompt: second nominal capacity with the same body.\n"
      "type Demo.Nominal.SecondCap = U64 <below(self, 100)>\n"
      "new // prompt: accept the first capacity.\n"
      "def Demo.Nominal.useFirst(cap: Demo.Nominal.FirstCap) <lessOrEqual(0, cap)>: Bool =\n"
      "  cap >= 0\n"
      "new // prompt: accept the second capacity.\n"
      "def Demo.Nominal.useSecond(cap: Demo.Nominal.SecondCap) <lessOrEqual(0, cap)>: Bool =\n"
      "  cap >= 0\n"
      "new // prompt: test that reaches both capacities.\n"
      "val Demo.Nominal.Tests.both {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(Demo.Nominal.useFirst(1) && Demo.Nominal.useSecond(1), true, \"both\")\n";
  CHECK(stigi_handle_frame(&srv_mcp, nominal, strlen(nominal), &rep, &rlen, &diag) == STIGI_OK, "nominal batch");
  CHECK(strstr(rep, "admitted component") != NULL, "two nominal types with equal bodies admitted");
  free(rep);
  int32_t first_ti = stigi_image_find(&door_mcp.image, "Demo.Nominal.FirstCap", 21);
  int32_t second_ti = stigi_image_find(&door_mcp.image, "Demo.Nominal.SecondCap", 22);
  CHECK(first_ti >= 0 && second_ti >= 0, "both nominal types in image");
  if (first_ti >= 0 && second_ti >= 0) {
    CHECK(memcmp(door_mcp.image.units[first_ti].unit_id, door_mcp.image.units[second_ti].unit_id, 32) != 0,
          "S136: two types with facts and equal bodies have different unit_id");
    CHECK(memcmp(door_mcp.image.units[first_ti].verdict_key, door_mcp.image.units[second_ti].verdict_key, 32) != 0,
          "S136: two types with facts and equal bodies have different verdict_key");
  }
  int32_t use_first_i = stigi_image_find(&door_mcp.image, "Demo.Nominal.useFirst", 21);
  int32_t use_second_i = stigi_image_find(&door_mcp.image, "Demo.Nominal.useSecond", 22);
  CHECK(use_first_i >= 0 && use_second_i >= 0, "both nominal users in image");
  if (use_first_i >= 0 && use_second_i >= 0) {
    CHECK(memcmp(door_mcp.image.units[use_first_i].unit_id, door_mcp.image.units[use_second_i].unit_id, 32) != 0,
          "S136: defs that take the two nominal types have different unit_id");
  }

  /* 9. Defect #25: 1,000-frame flat-RSS + zero-page-fault oracle with REAL `check` frames */
  const char *real_check =
      "check\n"
      "new // prompt: real check unit\n"
      "val Demo.Tests.ping {Stigi.Test}: Stigi.Test.Result = Stigi.Test.expect(5 + 5, 10, \"5 + 5 == 10\")\n";
  size_t real_check_len = strlen(real_check);
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  long rss_10 = 0;
  long rss_1000 = 0;
  struct rusage ru10, ru1000;
  memset(&ru10, 0, sizeof ru10);
  memset(&ru1000, 0, sizeof ru1000);
  for (int i = 1; i <= 1000; i++) {
    char *crep = NULL;
    size_t crl = 0;
    CHECK(stigi_handle_frame(&srv_mcp, real_check, real_check_len, &crep, &crl, &diag) == STIGI_OK,
          "1000-frame real check ok");
    CHECK(strstr(crep, "admitted component 1") != NULL, "real check admitted");
    free(crep);
    if (i == 10) {
      rss_10 = read_vm_rss_kb();
      (void)getrusage(RUSAGE_SELF, &ru10);
    }
    if (i == 1000) {
      rss_1000 = read_vm_rss_kb();
      (void)getrusage(RUSAGE_SELF, &ru1000);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double ms = (double)(t1.tv_sec - t0.tv_sec) * 1000.0 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
  long rss_delta = rss_1000 - rss_10;
  long minflt_delta = ru1000.ru_minflt - ru10.ru_minflt;
  long majflt_delta = ru1000.ru_majflt - ru10.ru_majflt;
  printf("c9 3-transport parity: PASS (6 mutating+read frames byte-identical + identical unit_ids across 3 servers)\n");
  printf("c9 negative controls + S119: PASS (all 8 controls verified)\n");
#if defined(__SANITIZE_ADDRESS__) || defined(__FILC__)
  const char *rss_note = " (unchecked under ASan/Fil-C)";
#else
  const char *rss_note = "";
#endif
  printf("c9 1000-frame real-check RSS: wall=%.2f ms, rss10=%ld KB, rss1000=%ld KB, delta=%ld KB, minflt=%ld, majflt=%ld%s\n",
         ms, rss_10, rss_1000, rss_delta, minflt_delta, majflt_delta, rss_note);

#if !defined(__SANITIZE_ADDRESS__) && !defined(__FILC__)
  CHECK(rss_delta <= 64, "RSS delta <= 64 KB over 1000 real check frames");
  CHECK(majflt_delta == 0, "0 major page faults in steady state");
  CHECK(minflt_delta == 0, "0 minor page faults in steady state");
#endif

  /* 10. Wave 4 controls: batch inline test, JSON-RPC scalar id, \u0000 rejection, T18 parallel==seq */
  {
    const char *inline_test_batch =
        "batch\n"
        "test (2 + 3) == 5\n";
    CHECK(stigi_handle_frame(&srv_mcp, inline_test_batch, strlen(inline_test_batch),
                             &rep, &rlen, &diag) == STIGI_OK,
          "batch inline test ok");
    CHECK(strstr(rep, "admitted component 1") != NULL,
          "batch inline test passes tenet gate with readable name");
    free(rep);

    const char *bad_obj_id = "{\"jsonrpc\":\"2.0\",\"id\":{\"a\":1},\"method\":\"ping\"}";
    CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, bad_obj_id, strlen(bad_obj_id),
                                   &rep, &rlen, &diag) == STIGI_OK,
          "invalid object id handled");
    CHECK(rep != NULL && strstr(rep, "-32600") != NULL,
          "JSON-RPC non-scalar object id rejected with -32600");
    free(rep);

    const char *nul_esc = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"stigi_frame\",\"arguments\":{\"frame\":\"list\\u0000\"}}}";
    CHECK(stigi_mcp_handle_jsonrpc(&srv_mcp, nul_esc, strlen(nul_esc),
                                   &rep, &rlen, &diag) == STIGI_OK,
          "\\u0000 escape handled");
    CHECK(rep != NULL && strstr(rep, "\"error\"") != NULL,
          "JSON string with \\u0000 embedded NUL rejected");
    free(rep);
  }

  /* S126 / S127: explicit treeshake / gc frame verbs return accepted gc */
  {
    char *rep = NULL;
    size_t rlen = 0;
    CHECK(stigi_handle_frame(&srv_sock, "treeshake", 9, &rep, &rlen, &diag) == STIGI_OK, "treeshake frame");
    CHECK(rep != NULL && strstr(rep, "accepted gc (removed=0") != NULL, "treeshake frame clean on live image");
    free(rep);
    rep = NULL;
    rlen = 0;
    CHECK(stigi_handle_frame(&srv_sock, "gc", 2, &rep, &rlen, &diag) == STIGI_OK, "gc frame");
    CHECK(rep != NULL && strstr(rep, "accepted gc (removed=0") != NULL, "gc frame clean on live image");
    free(rep);
  }

  /* T18 lock-free parallel vs sequential parity on 9 units (8 helpers + main) */
  {
    const char *par_srcs[9] = {
        "// prompt: pure math helper one.\npub def Par.helperOne(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 1\n",
        "// prompt: pure math helper two.\npub def Par.helperTwo(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 2\n",
        "// prompt: pure math helper three.\npub def Par.helperThree(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 3\n",
        "// prompt: pure math helper four.\npub def Par.helperFour(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 4\n",
        "// prompt: pure math helper five.\npub def Par.helperFive(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 5\n",
        "// prompt: pure math helper six.\npub def Par.helperSix(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 6\n",
        "// prompt: pure math helper seven.\npub def Par.helperSeven(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 7\n",
        "// prompt: pure math helper eight.\npub def Par.helperEight(value: U64) <lessOrEqual(0, value), lessOrEqual(value, 100)>: U64 = value + 8\n",
        "// prompt: entry calling all eight parallel helpers.\n"
        "pub def main(): I32 <lessOrEqual(0, result)> =\n"
        "  val first: U64 = Par.helperOne(1)\n"
        "  val second: U64 = Par.helperTwo(1)\n"
        "  val third: U64 = Par.helperThree(1)\n"
        "  val fourth: U64 = Par.helperFour(1)\n"
        "  val fifth: U64 = Par.helperFive(1)\n"
        "  val sixth: U64 = Par.helperSix(1)\n"
        "  val seventh: U64 = Par.helperSeven(1)\n"
        "  val eighth: U64 = Par.helperEight(1)\n"
        "  if first == 2 && second == 3 && third == 4 && fourth == 5 && fifth == 6 && sixth == 7 && seventh == 8 && eighth == 9 then 0 else 1\n"
    };
    size_t par_lens[9];
    for (int i = 0; i < 9; i++) par_lens[i] = strlen(par_srcs[i]);

    StigiDoor d_seq, d_par;
    CHECK(stigi_door_init(&d_seq, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK, "d_seq init");
    CHECK(stigi_door_init(&d_par, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK, "d_par init");

    StigiDoorResult r_seq, r_par;
    stigi_door_result_init(&r_seq);
    stigi_door_result_init(&r_par);
    setenv("STIGI_THREADS", "1", 1);
    CHECK(stigi_door_submit(&d_seq, par_srcs, par_lens, 9, &r_seq, &diag) == STIGI_OK && r_seq.accepted,
          "T18 sequential 9-unit submit accepted");
    setenv("STIGI_THREADS", "4", 1);
    CHECK(stigi_door_submit(&d_par, par_srcs, par_lens, 9, &r_par, &diag) == STIGI_OK && r_par.accepted,
          "T18 parallel 4-thread 9-unit submit accepted");
    unsetenv("STIGI_THREADS");

    CHECK(d_seq.image.unit_count == 9 && d_par.image.unit_count == 9, "T18 parallel == seq unit count");
    for (uint32_t i = 0; i < 9; i++) {
      CHECK(strcmp(d_seq.image.units[i].name, d_par.image.units[i].name) == 0 &&
            memcmp(d_seq.image.units[i].unit_id, d_par.image.units[i].unit_id, 32) == 0 &&
            memcmp(d_seq.image.units[i].verdict_key, d_par.image.units[i].verdict_key, 32) == 0,
            "T18 parallel == seq byte-identical runtime_id and verdict_key");
    }
    printf("c9 T18 parallel==seq: PASS (9 units, 4 lock-free workers byte-identical to 1 thread)\n");
    stigi_door_result_free(&r_seq);
    stigi_door_result_free(&r_par);
    stigi_door_free(&d_seq);
    stigi_door_free(&d_par);
  }

  /* Phase 2.4 (S127): comptime tree-shaking folds pure private calls, arithmetic, and prunes orphaned units */
  {
    StigiDoor ts_door;
    CHECK(stigi_door_init(&ts_door, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &diag) == STIGI_OK,
          "ts_door init");
    const char *ts3_srcs[3] = {
        ("// prompt: private pure helper.\n"
         "def Shake.privateFold(input: U64) <below(input, 100)>: U64 <lessOrEqual(input, result)> =\n"
         "  val doubled: U64 = input + input\n"
         "  doubled + 2\n"),
        ("// prompt: public wrapper calling privateFold with constant argument.\n"
         "pub def Shake.wrapper(): U64 <lessOrEqual(0, result)> =\n"
         "  Shake.privateFold(4)\n"),
        ("// prompt: external entry calling Shake.wrapper.\n"
         "pub def main(): I32 <lessOrEqual(0, result)> = if Shake.wrapper() == 10 then 0 else 1\n")
    };
    size_t ts3_lens[3] = {strlen(ts3_srcs[0]), strlen(ts3_srcs[1]), strlen(ts3_srcs[2])};
    StigiDoorResult tsr;
    stigi_door_result_init(&tsr);
    Status tss = stigi_door_submit(&ts_door, ts3_srcs, ts3_lens, 3, &tsr, &diag);
    if (!tsr.accepted) {
      for (uint32_t i = 0; i < tsr.reject_count; i++) {
        fprintf(stderr, "  ts3 reject: %s %s %s\n", tsr.rejects[i].subject, tsr.rejects[i].code, tsr.rejects[i].msg);
      }
    }
    CHECK(tss == STIGI_OK && tsr.accepted, "S127 submit private helper before treeshake");
    stigi_door_result_free(&tsr);
    CHECK(ts_door.image.unit_count == 3, "S127 image has 3 units before treeshake");

    uint32_t removed = 0;
    stigi_door_result_init(&tsr);
    CHECK(stigi_door_gc(&ts_door, &removed, &tsr, &diag) == STIGI_OK && tsr.accepted,
          "S127 treeshake folds private helper and prunes orphaned unit");
    stigi_door_result_free(&tsr);
    int32_t widx = stigi_image_find(&ts_door.image, "Shake.wrapper", strlen("Shake.wrapper"));
    CHECK(widx >= 0 && strstr(ts_door.unit_srcs[widx], "Shake.privateFold(") == NULL &&
              strstr(ts_door.unit_srcs[widx], "= 10") != NULL,
          "S127 treeshake folded pure private call Shake.privateFold(4) -> 10");
    CHECK(removed == 1U && ts_door.image.unit_count == 2U &&
              stigi_image_find(&ts_door.image, "Shake.privateFold", strlen("Shake.privateFold")) < 0,
          "S127 treeshake pruned orphaned Shake.privateFold after comptime evaluation");
    stigi_door_free(&ts_door);
  }

  stigi_server_free(&srv_sock);
  stigi_server_free(&srv_cc);
  stigi_server_free(&srv_mcp);
  stigi_door_free(&door_sock);
  stigi_door_free(&door_cc);
  stigi_door_free(&door_mcp);
  unlink(core_sock);
  unlink(cc_sock);
  return 0;
}

int main(void) {
  if (test_store_load_roundtrip() != 0) return 1;
  if (test_3transport_parity_and_negatives() != 0) return 1;
  if (test_cc_peer_identity() != 0) return 1;
  printf("c9_test: ALL PASS\n");
  return 0;
}
