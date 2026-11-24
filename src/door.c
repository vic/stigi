#define _POSIX_C_SOURCE 200809L

#include "door.h"
#include "blake3.h"
#include "eval.h"
#include "lex.h"
#include "parse.h"
#include "print.h"
#include "tenet_gate.h"
#include "types.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void stigi_door_result_init(StigiDoorResult *r) {
  if (r == NULL) return;
  memset(r, 0, sizeof *r);
  r->status = STIGI_OK;
}

void stigi_door_result_free(StigiDoorResult *r) {
  if (r == NULL) return;
  free(r->rejects);
  memset(r, 0, sizeof *r);
}

static Status add_reject(StigiDoorResult *out, const char *subject,
                         const char *code, const char *msg, Diag *diag) {
  if (out == NULL) return STIGI_OK;
  if (out->reject_count == out->reject_cap) {
    uint32_t ncap = out->reject_cap == 0 ? 8U : out->reject_cap * 2U;
    StigiDoorReject *nr = realloc(out->rejects, (size_t)ncap * sizeof *nr);
    if (nr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "reject list OOM");
    out->rejects = nr;
    out->reject_cap = ncap;
  }
  StigiDoorReject *r = &out->rejects[out->reject_count++];
  (void)snprintf(r->subject, sizeof r->subject, "%s", subject != NULL ? subject : "");
  (void)snprintf(r->code, sizeof r->code, "%s", code != NULL ? code : "");
  (void)snprintf(r->msg, sizeof r->msg, "%s", msg != NULL ? msg : "");
  return STIGI_OK;
}

static const char *tupu_code_to_stigi(int dl_code) {
  switch (dl_code) {
    case TUPU_CODE_DATALOG_ARITH: return "datalog-arith";
    case TUPU_CODE_ENGINE_OUT_OF_MEMORY: return "engine-out-of-memory";
    case TUPU_CODE_DATALOG_BUDGET: return "datalog-budget";
    case TUPU_CODE_RULE_UNSAFE: return "rule-unsafe";
    case TUPU_CODE_NEGATION_CYCLE: return "negation-cycle";
    case TUPU_CODE_RELATION_ARITY: return "relation-arity";
    case TUPU_CODE_UNRESOLVED_NAME: return "unresolved-name";
    default: return "datalog-budget";
  }
}

/* ---- O(1) open-addressed key set cache ---- */

static uint64_t key_hash64(const uint8_t key[32]) {
  uint64_t h = 0;
  for (int i = 0; i < 8; i++) {
    h |= ((uint64_t)key[i]) << (i * 8);
  }
  return h;
}

static void keyset_free(StigiKeySet *ks) {
  if (ks == NULL) return;
  free(ks->slots);
  ks->slots = NULL;
  ks->count = 0;
  ks->cap = 0;
}

static bool keyset_contains(const StigiKeySet *ks, const uint8_t key[32]) {
  if (ks == NULL || ks->cap == 0) return false;
  uint32_t mask = ks->cap - 1U;
  uint32_t idx = (uint32_t)(key_hash64(key) & mask);
  while (ks->slots[idx].used) {
    if (memcmp(ks->slots[idx].key, key, 32) == 0) return true;
    idx = (idx + 1U) & mask;
  }
  return false;
}

static Status keyset_rehash(StigiKeySet *ks, uint32_t new_cap, Diag *diag) {
  StigiCacheSlot *ns = calloc(new_cap, sizeof *ns);
  if (ns == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "keyset OOM");
  uint32_t mask = new_cap - 1U;
  for (uint32_t i = 0; i < ks->cap; i++) {
    if (!ks->slots[i].used) continue;
    uint32_t idx = (uint32_t)(key_hash64(ks->slots[i].key) & mask);
    while (ns[idx].used) idx = (idx + 1U) & mask;
    ns[idx] = ks->slots[i];
  }
  free(ks->slots);
  ks->slots = ns;
  ks->cap = new_cap;
  return STIGI_OK;
}

/* Pre-reserve enough capacity so subsequent atomic inserts cannot fail with OOM */
static Status keyset_reserve(StigiKeySet *ks, uint32_t extra, Diag *diag) {
  uint64_t need = (uint64_t)ks->count + (uint64_t)extra;
  if (need == 0) return STIGI_OK;
  uint32_t cap = ks->cap == 0 ? 64U : ks->cap;
  while ((uint64_t)cap * 3ULL <= need * 4ULL) {
    if (cap > UINT32_MAX / 2U) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "keyset overflow");
    cap *= 2U;
  }
  if (cap != ks->cap) return keyset_rehash(ks, cap, diag);
  return STIGI_OK;
}

static Status keyset_insert(StigiKeySet *ks, const uint8_t key[32], Diag *diag) {
  if (keyset_contains(ks, key)) return STIGI_OK;
  Status st = keyset_reserve(ks, 1U, diag);
  if (st != STIGI_OK) return st;
  uint32_t mask = ks->cap - 1U;
  uint32_t idx = (uint32_t)(key_hash64(key) & mask);
  while (ks->slots[idx].used) idx = (idx + 1U) & mask;
  memcpy(ks->slots[idx].key, key, 32);
  ks->slots[idx].used = true;
  ks->count++;
  return STIGI_OK;
}

/* ---- Decision store: records sorted by key, binary search (S134 e) ---- */

const StigiUnitDecision *stigi_decisions_find(const StigiDecisionStore *ds, const uint8_t key[32]) {
  uint32_t lo = 0, hi = ds->count;
  while (lo < hi) {
    uint32_t mid = lo + (hi - lo) / 2U;
    int c = memcmp(ds->recs[mid].key, key, 32);
    if (c == 0) return &ds->recs[mid].dec;
    if (c < 0) lo = mid + 1U;
    else hi = mid;
  }
  return NULL;
}

Status stigi_decisions_insert(StigiDecisionStore *ds, const uint8_t key[32], StigiUnitDecision *dec,
                              Diag *diag) {
  uint32_t lo = 0, hi = ds->count;
  while (lo < hi) {
    uint32_t mid = lo + (hi - lo) / 2U;
    int c = memcmp(ds->recs[mid].key, key, 32);
    if (c == 0) {
      free(dec->masked); /* the key is already stored: same key, same rows */
      dec->masked = NULL;
      return STIGI_OK;
    }
    if (c < 0) lo = mid + 1U;
    else hi = mid;
  }
  if (ds->count == ds->cap) {
    uint32_t ncap = ds->cap == 0 ? 64U : ds->cap * 2U;
    StigiDecisionRec *nr = realloc(ds->recs, (size_t)ncap * sizeof *nr);
    if (nr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "decision store OOM");
    ds->recs = nr;
    ds->cap = ncap;
  }
  memmove(&ds->recs[lo + 1U], &ds->recs[lo], (size_t)(ds->count - lo) * sizeof *ds->recs);
  memcpy(ds->recs[lo].key, key, 32);
  ds->recs[lo].dec = *dec;
  dec->masked = NULL;
  ds->count++;
  return STIGI_OK;
}

Status stigi_decisions_copy(const StigiDecisionStore *src, StigiDecisionStore *dst, Diag *diag) {
  memset(dst, 0, sizeof *dst);
  if (src->count == 0) return STIGI_OK;
  dst->recs = calloc(src->count, sizeof *dst->recs);
  if (dst->recs == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "decision snapshot OOM");
  dst->cap = src->count;
  for (uint32_t i = 0; i < src->count; i++) {
    dst->recs[i] = src->recs[i];
    dst->recs[i].dec.masked = NULL;
    if (src->recs[i].dec.access_count > 0) {
      dst->recs[i].dec.masked = malloc(src->recs[i].dec.access_count);
      if (dst->recs[i].dec.masked == NULL) {
        dst->count = i;
        stigi_decisions_free(dst);
        return diag_set(diag, STIGI_ERR_OOM, -1, "decision snapshot OOM");
      }
      memcpy(dst->recs[i].dec.masked, src->recs[i].dec.masked, src->recs[i].dec.access_count);
    }
    dst->count = i + 1U;
  }
  return STIGI_OK;
}

void stigi_decisions_free(StigiDecisionStore *ds) {
  if (ds == NULL) return;
  for (uint32_t i = 0; i < ds->count; i++) free(ds->recs[i].dec.masked);
  free(ds->recs);
  memset(ds, 0, sizeof *ds);
}

static Status decisions_reserve(StigiDecisionStore *ds, uint32_t extra, Diag *diag) {
  uint64_t need = (uint64_t)ds->count + extra;
  if (need <= ds->cap) return STIGI_OK;
  uint32_t ncap = ds->cap == 0 ? 64U : ds->cap;
  while (ncap < need) ncap *= 2U;
  StigiDecisionRec *nr = realloc(ds->recs, (size_t)ncap * sizeof *nr);
  if (nr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "decision store OOM");
  ds->recs = nr;
  ds->cap = ncap;
  return STIGI_OK;
}

/* Order-independent hash of base rules + sorted in-image Rule & Relation verdict_keys */
static int cmp_key32(const void *a, const void *b) {
  return memcmp(a, b, 32);
}

static Status compute_active_rules_hash(const StigiDoor *door, const StigiImage *img,
                                        uint8_t out[32], Diag *diag) {
  size_t rule_cnt = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    StigiDeclKind k = img->units[i].ast->decl->kind;
    if (k == STIGI_DECL_RULE || k == STIGI_DECL_RELATION) rule_cnt++;
  }
  size_t buf_len = 32U + rule_cnt * 32U;
  uint8_t *buf = malloc(buf_len);
  if (buf == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "rules hash OOM");
  memcpy(buf, door->base_rules_hash, 32);
  size_t off = 32U;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    StigiDeclKind k = img->units[i].ast->decl->kind;
    if (k == STIGI_DECL_RULE || k == STIGI_DECL_RELATION) {
      memcpy(buf + off, img->units[i].verdict_key, 32);
      off += 32U;
    }
  }
  if (rule_cnt > 1U) {
    qsort(buf + 32U, rule_cnt, 32U, cmp_key32);
  }
  blake3_hash(buf, buf_len, out);
  free(buf);
  return STIGI_OK;
}

/* Verdict key includes unit verdict_key, active rules hash, seed build id, row_budget, AND kernel_mode */
static void effective_verdict_key(const uint8_t base_key[32], const uint8_t rules_hash[32],
                                  const uint8_t seed_id[32], uint64_t budget, bool kernel_mode,
                                  uint8_t out[32]) {
  uint8_t buf[105];
  memcpy(buf, base_key, 32);
  memcpy(buf + 32, rules_hash, 32);
  memcpy(buf + 64, seed_id, 32);
  for (int i = 0; i < 8; i++) {
    buf[96 + i] = (uint8_t)((budget >> (i * 8)) & 0xFFU);
  }
  buf[104] = kernel_mode ? 1U : 0U;
  blake3_hash(buf, sizeof buf, out);
}

/* BLAKE3 of the running seed binary: any change of the C logic changes every verdict key.
   The binary does not change while the process runs: hash it once, streamed, no heap. */
static pthread_once_t seed_id_once = PTHREAD_ONCE_INIT;
static bool seed_id_ok = false;
static int seed_id_errno = 0;
static uint8_t seed_id_cached[32];

static void seed_id_compute_once(void) {
  FILE *f = fopen("/proc/self/exe", "rb");
  if (f == NULL) {
    seed_id_errno = errno;
    return;
  }
  StigiBlake3 h;
  blake3_init(&h);
  uint8_t buf[16384];
  size_t got;
  while ((got = fread(buf, 1, sizeof buf, f)) > 0) blake3_update(&h, buf, got);
  bool failed = ferror(f) != 0;
  seed_id_errno = errno;
  (void)fclose(f);
  if (failed) return;
  blake3_finalize(&h, seed_id_cached);
  seed_id_ok = true;
}

static Status compute_seed_id(uint8_t out[32], Diag *diag) {
  (void)pthread_once(&seed_id_once, seed_id_compute_once);
  if (!seed_id_ok) {
    return diag_set(diag, STIGI_ERR_IO, seed_id_errno, "cannot read /proc/self/exe for the seed id");
  }
  memcpy(out, seed_id_cached, 32);
  return STIGI_OK;
}

/* ---- Preprocessor for base all.dl ---- */

static Status preprocess_dl(const char *path, char **out_buf, size_t *out_len, Diag *diag) {
  if (path == NULL || path[0] == '\0') {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "preprocess_dl: empty path");
  }
  int pipefd[2];
  if (pipe(pipefd) != 0) {
    return diag_set(diag, STIGI_ERR_IO, errno, "pipe() failed for preprocessor");
  }

  size_t plen = strlen(path);
  char *path_copy = malloc(plen + 1U);
  if (path_copy == NULL) {
    (void)close(pipefd[0]);
    (void)close(pipefd[1]);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  memcpy(path_copy, path, plen + 1U);

  pid_t pid = fork();
  if (pid < 0) {
    int e = errno;
    free(path_copy);
    (void)close(pipefd[0]);
    (void)close(pipefd[1]);
    return diag_set(diag, STIGI_ERR_IO, e, "fork() failed for preprocessor");
  }
  if (pid == 0) {
    if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
    (void)close(pipefd[0]);
    (void)close(pipefd[1]);
    char arg0[] = "gcc";
    char arg1[] = "-E";
    char arg2[] = "-P";
    char arg3[] = "-x";
    char arg4[] = "c";
    char *const argv[] = {arg0, arg1, arg2, arg3, arg4, path_copy, NULL};
    execvp(arg0, argv);
    _exit(127);
  }

  free(path_copy);
  (void)close(pipefd[1]);

  size_t cap = 65536U;
  size_t len = 0;
  char *buf = malloc(cap);
  if (buf == NULL) {
    (void)close(pipefd[0]);
    int wst = 0;
    while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  for (;;) {
    if (len + 4096U + 1U > cap) {
      if (cap > SIZE_MAX / 2U) {
        free(buf);
        (void)close(pipefd[0]);
        int wst = 0;
        while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
        return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "preprocessor output overflow");
      }
      cap *= 2U;
      char *nb = realloc(buf, cap);
      if (nb == NULL) {
        free(buf);
        (void)close(pipefd[0]);
        int wst = 0;
        while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
        return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
      buf = nb;
    }
    ssize_t n = read(pipefd[0], buf + len, 4096U);
    if (n < 0) {
      if (errno == EINTR) continue;
      int e = errno;
      free(buf);
      (void)close(pipefd[0]);
      int wst = 0;
      while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
      return diag_set(diag, STIGI_ERR_IO, e, "read from preprocessor failed on %s", path);
    }
    if (n == 0) break;
    len += (size_t)n;
  }
  (void)close(pipefd[0]);

  int wstatus = 0;
  while (waitpid(pid, &wstatus, 0) < 0) {
    if (errno == EINTR) continue;
    int e = errno;
    free(buf);
    return diag_set(diag, STIGI_ERR_IO, e, "waitpid failed for preprocessor on %s", path);
  }
  if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
    int code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;
    free(buf);
    return diag_set(diag, STIGI_ERR_IO, code, "preprocessor failed on %s", path);
  }
  buf[len] = '\0';
  *out_buf = buf;
  *out_len = len;
  return STIGI_OK;
}

/* ---- Dynamic string builder for in-image Datalog rule/relation compilation ---- */

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} DlBuf;

static void db_mem(DlBuf *b, const char *s, size_t n) {
  if (b->oom || n == 0) return;
  if (n > SIZE_MAX - b->len - 1U) { b->oom = true; return; }
  if (b->len + n + 1U > b->cap) {
    size_t ncap = b->cap == 0 ? 4096U : b->cap;
    while (ncap < b->len + n + 1U) {
      if (ncap > SIZE_MAX / 2U) { b->oom = true; return; }
      ncap *= 2U;
    }
    char *nb = realloc(b->buf, ncap);
    if (nb == NULL) { b->oom = true; return; }
    b->buf = nb;
    b->cap = ncap;
  }
  memcpy(b->buf + b->len, s, n);
  b->len += n;
  b->buf[b->len] = '\0';
}

static void db_cstr(DlBuf *b, const char *s) {
  if (s != NULL) db_mem(b, s, strlen(s));
}

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool is_dl_num_type(StigiStr name) {
  return str_eq_c(name, "Stigi.Integer") || str_eq_c(name, "U8") || str_eq_c(name, "I8") ||
         str_eq_c(name, "U32") || str_eq_c(name, "I32") || str_eq_c(name, "U64") ||
         str_eq_c(name, "I64");
}

static bool dl_decl_exists(const char *dl_src, const char *rel_name) {
  if (dl_src == NULL || rel_name == NULL) return false;
  char pat[160];
  int n = snprintf(pat, sizeof pat, ".decl %s(", rel_name);
  if (n <= 0 || (size_t)n >= sizeof pat) return false;
  return strstr(dl_src, pat) != NULL;
}

static void resolve_rel_dl_name(const StigiImage *img, uint32_t from_idx, const char *base_dl,
                                StigiStr raw_name, char out[160]) {
  /* Resolve aliases/in-image relation first */
  int32_t ridx = stigi_image_resolve_name(img, from_idx, raw_name.ptr, raw_name.len);
  const char *src = raw_name.ptr;
  size_t len = raw_name.len;
  if (ridx >= 0) {
    src = img->units[ridx].name;
    len = img->units[ridx].name_len;
  }
  /* Check if plain suffix (after Stigi. / Stigi.Facts.) already exists in schema.dl */
  const char *last_dot = NULL;
  for (size_t i = 0; i < len; i++) {
    if (src[i] == '.') last_dot = src + i;
  }
  if (last_dot != NULL && len >= 6 && memcmp(src, "Stigi.", 6) == 0) {
    const char *suf = last_dot + 1;
    size_t slen = (size_t)((src + len) - suf);
    if (slen < 150) {
      char tmp[160];
      memcpy(tmp, suf, slen);
      tmp[slen] = '\0';
      if (dl_decl_exists(base_dl, tmp)) {
        memcpy(out, tmp, slen + 1U);
        return;
      }
    }
  }
  if (last_dot == NULL && len < 150) {
    char tmp[160];
    memcpy(tmp, src, len);
    tmp[len] = '\0';
    if (strcmp(tmp, "lessOrEqual") == 0 && dl_decl_exists(base_dl, "leq")) {
      memcpy(out, "leq", 4U);
      return;
    }
    if (strcmp(tmp, "notEqual") == 0 && dl_decl_exists(base_dl, "neq")) {
      memcpy(out, "neq", 4U);
      return;
    }
    if (dl_decl_exists(base_dl, tmp)) {
      memcpy(out, tmp, len + 1U);
      return;
    }
  }
  /* Mangle dotted Stigi relation name into valid Datalog identifier */
  size_t pos = 0;
  for (size_t i = 0; i < len && pos + 3 < 160; i++) {
    if (src[i] == '.') {
      out[pos++] = '_';
      out[pos++] = '_';
    } else {
      out[pos++] = src[i];
    }
  }
  out[pos] = '\0';
}

static void emit_rule_expr_dl(DlBuf *b, const StigiExpr *e) {
  if (e == NULL) { db_cstr(b, "_"); return; }
  if (e->kind == STIGI_EXPR_LOCAL || e->kind == STIGI_EXPR_GLOBAL) {
    db_mem(b, e->u.ident.name.ptr, e->u.ident.name.len);
  } else if (e->kind == STIGI_EXPR_LIT_INT) {
    char nb[32];
    int nn = snprintf(nb, sizeof nb, "%llu", (unsigned long long)e->u.lit_int.value);
    if (nn > 0) db_mem(b, nb, (size_t)nn);
  } else if (e->kind == STIGI_EXPR_LIT_STR) {
    db_mem(b, e->u.lit_str.raw.ptr, e->u.lit_str.raw.len);
  } else if (e->kind == STIGI_EXPR_UNOP && e->u.unop.op == STIGI_OP_NEG) {
    db_cstr(b, "-");
    emit_rule_expr_dl(b, e->u.unop.operand);
  } else if (e->kind == STIGI_EXPR_BINOP) {
    db_cstr(b, "(");
    emit_rule_expr_dl(b, e->u.binop.lhs);
    switch (e->u.binop.op) {
      case STIGI_OP_ADD: db_cstr(b, " + "); break;
      case STIGI_OP_SUB: db_cstr(b, " - "); break;
      case STIGI_OP_MUL: db_cstr(b, " * "); break;
      case STIGI_OP_DIV: db_cstr(b, " / "); break;
      case STIGI_OP_MOD: db_cstr(b, " % "); break;
      default: db_cstr(b, " + "); break;
    }
    emit_rule_expr_dl(b, e->u.binop.rhs);
    db_cstr(b, ")");
  } else {
    db_cstr(b, "_");
  }
}

/* Order atoms of a rule unit (ruling S138, Option A of p1-patch.md 7): the kernel stores each order row once at its
   defining scope (belowFrom, leqFrom, upperFrom, lowerFrom); a reader at scope s sees a row stored at u when u
   dominates s. */
typedef enum { ORDER_NONE = 0, ORDER_BELOW, ORDER_LEQ, ORDER_UPPER, ORDER_LOWER } OrderAtom;
static const char *const ORDER_FROM_REL[] = {[ORDER_NONE] = "", [ORDER_BELOW] = "belowFrom",
                                             [ORDER_LEQ] = "leqFrom", [ORDER_UPPER] = "upperFrom",
                                             [ORDER_LOWER] = "lowerFrom"};

static StigiStr strip_facts_prefix(StigiStr n) {
  static const char pre[] = "Stigi.Facts.";
  size_t pl = sizeof pre - 1U;
  if (n.len > pl && memcmp(n.ptr, pre, pl) == 0) {
    n.ptr += pl;
    n.len -= pl;
  }
  return n;
}

static OrderAtom order_atom_of(StigiStr raw) {
  StigiStr n = strip_facts_prefix(raw);
  if (str_eq_c(n, "below")) return ORDER_BELOW;
  if (str_eq_c(n, "lessOrEqual") || str_eq_c(n, "leq")) return ORDER_LEQ;
  if (str_eq_c(n, "upper")) return ORDER_UPPER;
  if (str_eq_c(n, "lower")) return ORDER_LOWER;
  return ORDER_NONE;
}

/* Relations of the P1 order closure: kernel-internal, never named by a rule unit. */
static bool is_p1_internal_name(StigiStr raw) {
  static const char *const names[] = {"belowFrom", "leqFrom", "upperFrom", "lowerFrom", "upperBaseFrom",
                                      "lowerBaseFrom", "belowAt", "leqAt", "upperAt", "lowerAt",
                                      "orderAsk", "boundAsk", "scopeSpan", "spanBad", "hasSpan",
                                      "spansValid", "anySpanBad", "rootScope", "hasParent", "dom"};
  StigiStr n = strip_facts_prefix(raw);
  for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
    if (str_eq_c(n, names[i])) return true;
  }
  return false;
}

static void emit_rule_item_dl(DlBuf *b, const StigiImage *img, uint32_t from_idx,
                              const char *base_dl, const StigiFactItem *it, uint32_t *fresh,
                              bool is_head) {
  OrderAtom oa = it->kind == STIGI_FACT_ATOM ? order_atom_of(it->atom_name) : ORDER_NONE;
  if (oa != ORDER_NONE && !it->is_negated && !is_head && it->arg_count == 3) {
    /* below(a, b, s) -> belowFrom(a, b, _dU<n>), dom(s, _dU<n>); `_d` is a prefix the tenet gate refuses in
       source, so the fresh variable never meets a user name */
    char var[24];
    int vn = snprintf(var, sizeof var, "_dU%u", (*fresh)++);
    db_cstr(b, ORDER_FROM_REL[oa]);
    db_cstr(b, "(");
    emit_rule_expr_dl(b, it->args[0]);
    db_cstr(b, ", ");
    emit_rule_expr_dl(b, it->args[1]);
    db_cstr(b, ", ");
    if (vn > 0) db_mem(b, var, (size_t)vn);
    db_cstr(b, "), dom(");
    emit_rule_expr_dl(b, it->args[2]);
    db_cstr(b, ", ");
    if (vn > 0) db_mem(b, var, (size_t)vn);
    db_cstr(b, ")");
    return;
  }
  if (it->kind == STIGI_FACT_ATOM) {
    if (it->is_negated) db_cstr(b, "!");
    char rname[160];
    resolve_rel_dl_name(img, from_idx, base_dl, it->atom_name, rname);
    db_cstr(b, rname);
    db_cstr(b, "(");
    for (uint32_t i = 0; i < it->arg_count; i++) {
      if (i > 0) db_cstr(b, ", ");
      emit_rule_expr_dl(b, it->args[i]);
    }
    db_cstr(b, ")");
  } else if (it->kind == STIGI_FACT_CMP) {
    emit_rule_expr_dl(b, it->lhs);
    switch (it->op) {
      case STIGI_OP_EQ: db_cstr(b, " = "); break;
      case STIGI_OP_NE: db_cstr(b, " != "); break;
      case STIGI_OP_LT: db_cstr(b, " < "); break;
      case STIGI_OP_LE: db_cstr(b, " <= "); break;
      case STIGI_OP_GT: db_cstr(b, " > "); break;
      case STIGI_OP_GE: db_cstr(b, " >= "); break;
      default: db_cstr(b, " = "); break;
    }
    emit_rule_expr_dl(b, it->rhs);
  }
}

Status stigi_door_compile_rules(const StigiDoor *door, const StigiImage *cand,
                               char **out_dl, size_t *out_len, bool *has_custom, Diag *diag) {
  *out_dl = NULL;
  *out_len = 0;
  *has_custom = false;
  for (uint32_t i = 0; i < cand->unit_count; i++) {
    StigiDeclKind k = cand->units[i].ast->decl->kind;
    if (k == STIGI_DECL_RELATION || k == STIGI_DECL_RULE) {
      *has_custom = true;
      break;
    }
  }
  if (!*has_custom) return STIGI_OK;

  DlBuf db = {NULL, 0, 0, false};
  db_mem(&db, door->base_rules_dl, door->base_rules_dl_len);
  db_cstr(&db, "\n");

  /* 1. Emit .decl + .input for in-image relations not already in schema.dl */
  for (uint32_t i = 0; i < cand->unit_count && !db.oom; i++) {
    const StigiDecl *d = cand->units[i].ast->decl;
    if (d->kind != STIGI_DECL_RELATION) continue;
    StigiStr un = {cand->units[i].name, cand->units[i].name_len};
    char rname[160];
    resolve_rel_dl_name(cand, i, door->base_rules_dl, un, rname);
    if (dl_decl_exists(door->base_rules_dl, rname)) continue;
    db_cstr(&db, ".decl ");
    db_cstr(&db, rname);
    db_cstr(&db, "(");
    for (uint32_t c = 0; c < d->u.relation_decl.column_count; c++) {
      if (c > 0) db_cstr(&db, ", ");
      const StigiParam *cp = &d->u.relation_decl.columns[c];
      if (cp->name.len > 0) db_mem(&db, cp->name.ptr, cp->name.len);
      else {
        char cn[16];
        int nn = snprintf(cn, sizeof cn, "c%u", c);
        if (nn > 0) db_mem(&db, cn, (size_t)nn);
      }
      db_cstr(&db, ": ");
      if (cp->type != NULL && is_dl_num_type(cp->type->name)) db_cstr(&db, "number");
      else db_cstr(&db, "symbol");
    }
    db_cstr(&db, ")\n.input ");
    db_cstr(&db, rname);
    db_cstr(&db, "\n");
  }

  /* 2. Emit rules */
  for (uint32_t i = 0; i < cand->unit_count && !db.oom; i++) {
    const StigiDecl *d = cand->units[i].ast->decl;
    if (d->kind != STIGI_DECL_RULE) continue;
    uint32_t fresh = 0; /* per-rule counter of the _dU<n> scope variables */
    emit_rule_item_dl(&db, cand, i, door->base_rules_dl, &d->u.rule_decl.head, &fresh, true);
    if (d->u.rule_decl.body_count > 0) {
      db_cstr(&db, " :- ");
      for (uint32_t b = 0; b < d->u.rule_decl.body_count; b++) {
        if (b > 0) db_cstr(&db, ", ");
        emit_rule_item_dl(&db, cand, i, door->base_rules_dl, &d->u.rule_decl.body[b], &fresh, false);
      }
    }
    db_cstr(&db, ".\n");
  }

  if (db.oom) {
    free(db.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_door_compile_rules OOM");
  }
  *out_dl = db.buf;
  *out_len = db.len;
  return STIGI_OK;
}

/* ---- Door lifecycle ---- */

Status stigi_door_init(StigiDoor *door, const char *all_dl_path,
                       uint64_t row_budget, bool kernel_mode, Diag *diag) {
  if (door == NULL || all_dl_path == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_init: invalid arguments");
  }
  memset(door, 0, sizeof *door);
  stigi_image_init(&door->image);
  door->row_budget = row_budget > 0 ? row_budget : STIGI_DEFAULT_ROW_BUDGET;
  door->kernel_mode = kernel_mode;

  Status st = preprocess_dl(all_dl_path, &door->base_rules_dl, &door->base_rules_dl_len, diag);
  if (st != STIGI_OK) {
    stigi_door_free(door);
    return st;
  }
  door->rules_dl = malloc(door->base_rules_dl_len + 1U);
  if (door->rules_dl == NULL) {
    stigi_door_free(door);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  memcpy(door->rules_dl, door->base_rules_dl, door->base_rules_dl_len + 1U);
  door->rules_dl_len = door->base_rules_dl_len;

  blake3_hash((const uint8_t *)door->base_rules_dl, door->base_rules_dl_len,
              door->base_rules_hash);
  st = compute_seed_id(door->seed_id, diag);
  if (st != STIGI_OK) {
    stigi_door_free(door);
    return st;
  }
  st = stigi_dl_open(&door->dl, door->rules_dl, door->rules_dl_len, "all.dl",
                     door->row_budget, 1, diag);
  if (st != STIGI_OK) {
    stigi_door_free(door);
    return st;
  }
  return STIGI_OK;
}

Status stigi_door_prune_caches(StigiDoor *door, Diag *diag) {
  uint8_t rules_hash[32];
  Status st = compute_active_rules_hash(door, &door->image, rules_hash, diag);
  if (st != STIGI_OK) return st;
  StigiKeySet kept_verdicts = {NULL, 0, 0};
  StigiKeySet kept_tests = {NULL, 0, 0};
  StigiDecisionStore kept_decs = {NULL, 0, 0};
  for (uint32_t i = 0; i < door->image.unit_count && st == STIGI_OK; i++) {
    uint8_t key[32];
    effective_verdict_key(door->image.units[i].verdict_key, rules_hash, door->seed_id,
                          door->row_budget, door->kernel_mode, key);
    if (keyset_contains(&door->verdict_cache, key)) st = keyset_insert(&kept_verdicts, key, diag);
    if (st == STIGI_OK && keyset_contains(&door->verdict_cache, key)) {
      const StigiUnitDecision *old_dec = stigi_decisions_find(&door->decisions, key);
      if (old_dec != NULL) {
        StigiUnitDecision copy = *old_dec;
        copy.masked = NULL;
        if (old_dec->access_count > 0) {
          copy.masked = malloc(old_dec->access_count);
          if (copy.masked == NULL) st = diag_set(diag, STIGI_ERR_OOM, -1, "decision copy OOM");
          else memcpy(copy.masked, old_dec->masked, old_dec->access_count);
        }
        if (st == STIGI_OK) st = stigi_decisions_insert(&kept_decs, key, &copy, diag);
        if (st != STIGI_OK) free(copy.masked);
      }
    }
    if (st == STIGI_OK && keyset_contains(&door->test_cache, key)) {
      st = keyset_insert(&kept_tests, key, diag);
    }
  }
  if (st != STIGI_OK) {
    keyset_free(&kept_verdicts);
    keyset_free(&kept_tests);
    stigi_decisions_free(&kept_decs);
    return st;
  }
  keyset_free(&door->verdict_cache);
  keyset_free(&door->test_cache);
  door->verdict_cache = kept_verdicts;
  door->test_cache = kept_tests;
  stigi_decisions_free(&door->decisions);
  door->decisions = kept_decs;
  return STIGI_OK;
}

void stigi_door_free(StigiDoor *door) {
  if (door == NULL) return;
  stigi_image_free(&door->image);
  for (uint32_t i = 0; i < door->src_count; i++) {
    free(door->unit_srcs[i]);
  }
  free(door->unit_srcs);
  free(door->unit_lens);
  stigi_dl_close(&door->dl);
  free(door->base_rules_dl);
  free(door->rules_dl);
  keyset_free(&door->verdict_cache);
  keyset_free(&door->test_cache);
  stigi_decisions_free(&door->decisions);
  memset(door, 0, sizeof *door);
}

static void code_to_kebab(StigiStr code_ident, char *out, size_t cap) {
  const char *last_dot = NULL;
  for (size_t i = 0; i < code_ident.len; i++) {
    if (code_ident.ptr[i] == '.') last_dot = code_ident.ptr + i;
  }
  const char *s = last_dot != NULL ? last_dot + 1 : code_ident.ptr;
  size_t len = last_dot != NULL ? (size_t)((code_ident.ptr + code_ident.len) - s) : code_ident.len;
  size_t pos = 0;
  for (size_t i = 0; i < len && pos + 2 < cap; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c >= 'A' && c <= 'Z') {
      if (i > 0) out[pos++] = '-';
      out[pos++] = (char)(c - 'A' + 'a');
    } else {
      out[pos++] = (char)c;
    }
  }
  out[pos] = '\0';
}

/* Strict parser checking if a real `cleared(..., "code")` rule head exists in rules_dl */
static bool code_has_clearing_rule_in(const char *rules_dl, const char *code) {
  if (rules_dl == NULL || code == NULL || code[0] == '\0') return false;
  size_t clen = strlen(code);

  /* Spec variable-cleared codes (rules.md 10): cleared(o, c) :- belowOwed/leqOwed / kernelOnly */
  static const char *const k_order_codes[] = {
      "precondition-unproved", "exit-unproved", "assertion-unproved",
      "invariant-unproved", "refinement-unproved"};
  for (size_t i = 0; i < sizeof(k_order_codes) / sizeof(k_order_codes[0]); i++) {
    if (strcmp(code, k_order_codes[i]) == 0) {
      if (strstr(rules_dl, "cleared(o, c) :- leqOwed(") != NULL ||
          strstr(rules_dl, "cleared(o, c) :- belowOwed(") != NULL) {
        return true;
      }
    }
  }
  static const char *const k_kernel_codes[] = {"reserved-name", "extern-denied", "top-level-name"};
  for (size_t i = 0; i < sizeof(k_kernel_codes) / sizeof(k_kernel_codes[0]); i++) {
    if (strcmp(code, k_kernel_codes[i]) == 0) {
      if (strstr(rules_dl, "cleared(u, c) :- kernelOnly(") != NULL) return true;
    }
  }

  /* Door-sweep codes have no Datalog clearing rule by design (S28, S124) */
  if (strcmp(code, "fact-idle") == 0 || strcmp(code, "rule-idle") == 0) {
    return true;
  }

  /* Scan rule heads: line/statement starts with `cleared(` followed by `arg0, "<code>")` then `:-` or `.` */
  const char *p = rules_dl;
  while (*p != '\0') {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (p[0] == '/' && p[1] == '/') {
      while (*p != '\0' && *p != '\n') p++;
      continue;
    }
    if (strncmp(p, "cleared(", 8) == 0) {
      const char *q = p + 8;
      while (*q != '\0' && *q != ',' && *q != ')' && *q != '\n') q++;
      if (*q == ',') {
        q++;
        while (*q == ' ' || *q == '\t') q++;
        if (*q == '"' && strncmp(q + 1, code, clen) == 0 && q[1 + clen] == '"') {
          const char *r = q + 2 + clen;
          while (*r == ' ' || *r == '\t') r++;
          if (*r == ')') {
            r++;
            while (*r == ' ' || *r == '\t') r++;
            if ((r[0] == ':' && r[1] == '-') || r[0] == '.') {
              return true;
            }
          }
        }
      }
    }
    while (*p != '\0' && *p != '\n') p++;
  }
  return false;
}

/* ---- Boundary checks: kernel/user mode, extern-outside-handler, safety-reads-user ---- */

static bool is_kernel_head_name(const StigiImage *img, uint32_t from_idx, StigiStr raw_name) {
  if (raw_name.len >= 6 && memcmp(raw_name.ptr, "Stigi.", 6) == 0) return true;
  /* Check unit aliases */
  const StigiUnit *u = img->units[from_idx].ast;
  for (uint32_t a = 0; a < u->alias_count; a++) {
    if (u->aliases[a].name.len == raw_name.len &&
        memcmp(u->aliases[a].name.ptr, raw_name.ptr, raw_name.len) == 0) {
      if (u->aliases[a].target.len >= 6 && memcmp(u->aliases[a].target.ptr, "Stigi.", 6) == 0) {
        return true;
      }
    }
  }
  int32_t idx = stigi_image_resolve_name(img, from_idx, raw_name.ptr, raw_name.len);
  if (idx >= 0 && img->units[idx].name_len >= 6 && memcmp(img->units[idx].name, "Stigi.", 6) == 0) {
    return true;
  }
  /* Direct schema.dl safety heads, the order aliases and the P1 order-closure relations (S138) */
  if (str_eq_c(raw_name, "cleared") || str_eq_c(raw_name, "owes") ||
      str_eq_c(raw_name, "reject") || order_atom_of(raw_name) != ORDER_NONE ||
      is_p1_internal_name(raw_name)) {
    return true;
  }
  return false;
}

static bool fact_item_is_vacuous(const StigiFactItem *it) {
  if (it == NULL) return false;
  if (it->kind == STIGI_FACT_ATOM && it->arg_count == 2 &&
      (str_eq_c(it->atom_name, "lessOrEqual") || str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual") ||
       str_eq_c(it->atom_name, "below") || str_eq_c(it->atom_name, "Stigi.Facts.below"))) {
    const StigiExpr *a = it->args[0];
    const StigiExpr *b = it->args[1];
    if (a != NULL && b != NULL) {
      if (a->kind == STIGI_EXPR_LIT_INT && b->kind == STIGI_EXPR_LIT_INT) return true;
      if ((str_eq_c(it->atom_name, "lessOrEqual") ||
           str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual")) &&
          (a->kind == STIGI_EXPR_LOCAL || a->kind == STIGI_EXPR_GLOBAL) &&
          (b->kind == STIGI_EXPR_LOCAL || b->kind == STIGI_EXPR_GLOBAL) &&
          a->u.ident.name.len == b->u.ident.name.len &&
          memcmp(a->u.ident.name.ptr, b->u.ident.name.ptr, a->u.ident.name.len) == 0) {
        return true;
      }
    }
  }
  if (it->kind == STIGI_FACT_CMP && it->lhs != NULL && it->rhs != NULL) {
    if (it->lhs->kind == STIGI_EXPR_LIT_INT && it->rhs->kind == STIGI_EXPR_LIT_INT) return true;
  }
  return false;
}

static bool fact_group_has_vacuous(const StigiFactGroup *fg) {
  if (fg == NULL) return false;
  for (uint32_t i = 0; i < fg->count; i++) {
    if (fact_item_is_vacuous(&fg->items[i])) return true;
  }
  return false;
}

static bool expr_has_vacuous_fact(const StigiExpr *e, int depth) {
  if (e == NULL || depth > 128) return false;
  switch (e->kind) {
    case STIGI_EXPR_ASSERT:
      if (fact_group_has_vacuous(&e->u.assert_expr.facts)) return true;
      return expr_has_vacuous_fact(e->u.assert_expr.expr, depth + 1);
    case STIGI_EXPR_VAR:
      if (e->u.var_stmt.has_inv && fact_group_has_vacuous(&e->u.var_stmt.inv)) return true;
      return expr_has_vacuous_fact(e->u.var_stmt.init, depth + 1);
    case STIGI_EXPR_VAL:
      return expr_has_vacuous_fact(e->u.val_stmt.init, depth + 1);
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (expr_has_vacuous_fact(e->u.block.stmts[i], depth + 1)) return true;
      }
      return false;
    case STIGI_EXPR_IF:
      return expr_has_vacuous_fact(e->u.if_expr.cond, depth + 1) ||
             expr_has_vacuous_fact(e->u.if_expr.then_br, depth + 1) ||
             expr_has_vacuous_fact(e->u.if_expr.else_br, depth + 1);
    case STIGI_EXPR_WHILE:
      return expr_has_vacuous_fact(e->u.while_expr.cond, depth + 1) ||
             expr_has_vacuous_fact(e->u.while_expr.body, depth + 1);
    case STIGI_EXPR_MATCH:
      if (expr_has_vacuous_fact(e->u.match_expr.scrutinee, depth + 1)) return true;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (expr_has_vacuous_fact(e->u.match_expr.arms[i].body, depth + 1)) return true;
      }
      return false;
    case STIGI_EXPR_HANDLE:
      return expr_has_vacuous_fact(e->u.handle_stmt.body, depth + 1);
    default:
      return false;
  }
}

static Status check_boundary_rules(const StigiImage *cand, bool kernel_mode,
                                   StigiDoorResult *out, Diag *diag) {
  Status st = STIGI_OK;
  for (uint32_t i = 0; i < cand->unit_count && st == STIGI_OK; i++) {
    const StigiImageUnit *u = &cand->units[i];
    const StigiDecl *d = u->ast->decl;
    bool under_stigi = (u->name_len >= 6 && memcmp(u->name, "Stigi.", 6) == 0);
    bool has_dot = (memchr(u->name, '.', u->name_len) != NULL);

    if (!kernel_mode) {
      if (under_stigi) {
        st = add_reject(out, u->name, "reserved-name",
                        "Stigi.* units are only allowed in kernel mode", diag);
        if (st != STIGI_OK) break;
      } else if (d->kind == STIGI_DECL_EXTERN) {
        st = add_reject(out, u->name, "extern-denied",
                        "extern units are only allowed in kernel mode", diag);
        if (st != STIGI_OK) break;
      } else if (!has_dot && strcmp(u->name, "main") != 0) {
        st = add_reject(out, u->name, "top-level-name",
                        "top-level names other than main require kernel mode", diag);
        if (st != STIGI_OK) break;
      }
    }

    if (d->kind == STIGI_DECL_RULE) {
      if (d->u.rule_decl.head.kind == STIGI_FACT_ATOM &&
          is_kernel_head_name(cand, i, d->u.rule_decl.head.atom_name) && !under_stigi) {
        st = add_reject(out, u->name, "rule-outside-kernel",
                        "rules targeting Stigi.* relations must live under Stigi.*", diag);
        if (st != STIGI_OK) break;
      }
      /* S138: order rows are derived by the kernel only, readers go through dom, the closure is internal */
      const StigiFactItem *hd = &d->u.rule_decl.head;
      if (hd->kind == STIGI_FACT_ATOM &&
          (order_atom_of(hd->atom_name) != ORDER_NONE || is_p1_internal_name(hd->atom_name))) {
        st = add_reject(out, u->name, "rule-unsafe",
                        "a rule head may not name an order relation or a P1 order-closure relation", diag);
        if (st != STIGI_OK) break;
      }
      for (uint32_t b = 0; b < d->u.rule_decl.body_count && st == STIGI_OK; b++) {
        const StigiFactItem *bi = &d->u.rule_decl.body[b];
        if (bi->kind != STIGI_FACT_ATOM) continue;
        if (bi->is_negated && order_atom_of(bi->atom_name) != ORDER_NONE) {
          st = add_reject(out, u->name, "rule-unsafe",
                          "a negated order atom is refused: order rows hold by dominance, not by absence", diag);
          break;
        }
        if (is_p1_internal_name(bi->atom_name)) {
          st = add_reject(out, u->name, "rule-unsafe",
                          "a rule body may not name a P1 order-closure relation", diag);
          break;
        }
        if (str_eq_c(bi->atom_name, "owes") || str_eq_c(bi->atom_name, "cleared") ||
            str_eq_c(bi->atom_name, "reject") || str_eq_c(bi->atom_name, "Stigi.Gate.owes") ||
            str_eq_c(bi->atom_name, "Stigi.Gate.cleared") ||
            str_eq_c(bi->atom_name, "Stigi.Gate.reject")) {
          st = add_reject(out, u->name, "negation-cycle",
                          "rule body may not read owes, cleared, or reject", diag);
          break;
        }
        if (under_stigi) {
          int32_t ridx = stigi_image_resolve_name(cand, i, bi->atom_name.ptr, bi->atom_name.len);
          if (ridx >= 0 && cand->units[ridx].ast->decl->kind == STIGI_DECL_RELATION &&
              !(cand->units[ridx].name_len >= 6 &&
                memcmp(cand->units[ridx].name, "Stigi.", 6) == 0)) {
            st = add_reject(out, u->name, "safety-reads-user",
                            "kernel rule cannot read user relation", diag);
            break;
          }
        }
      }
    }
  }

  /* extern-outside-handler (names.dl 46-52): only Handler units or Stigi.* may reference body-less externs */
  for (uint32_t e = 0; e < cand->edge_count && st == STIGI_OK; e++) {
    uint32_t f = cand->edges[e].from;
    uint32_t t = cand->edges[e].to;
    if (f >= cand->unit_count || t >= cand->unit_count) continue;
    const StigiDecl *td = cand->units[t].ast->decl;
    if (td->kind == STIGI_DECL_EXTERN && !td->u.extern_decl.is_val &&
        td->u.extern_decl.body == NULL) {
      const StigiImageUnit *fu = &cand->units[f];
      bool from_stigi = (fu->name_len >= 6 && memcmp(fu->name, "Stigi.", 6) == 0);
      bool from_handler = (fu->ast->decl->kind == STIGI_DECL_HANDLER);
      if (!from_stigi && !from_handler) {
        st = add_reject(out, fu->name, "extern-outside-handler",
                        "body-less extern may only be called from a handler or Stigi.*", diag);
      }
    }
  }
  /* S124: reject vacuous / tautological facts (`<lessOrEqual(0, 1)>`, `<lessOrEqual(x, x)>`) as fact-idle */
  for (uint32_t i = 0; i < cand->unit_count && st == STIGI_OK; i++) {
    const StigiImageUnit *u = &cand->units[i];
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_DEF) continue;
    const StigiDefHead *h = &d->u.def_decl.head;
    bool idle = false;
    for (uint32_t g = 0; g < h->pre_group_count && !idle; g++) {
      if (h->pre_groups[g].is_fact && fact_group_has_vacuous(&h->pre_groups[g].facts)) idle = true;
    }
    for (uint32_t g = 0; g < h->post_val_fact_count && !idle; g++) {
      if (fact_group_has_vacuous(&h->post_val_facts[g])) idle = true;
    }
    for (uint32_t g = 0; g < h->exit_fact_count && !idle; g++) {
      if (fact_group_has_vacuous(&h->exit_facts[g])) idle = true;
    }
    if (!idle && expr_has_vacuous_fact(stigi_def_body(d), 0)) idle = true;
    if (idle) {
      st = add_reject(out, u->name, "fact-idle",
                      "vacuous or tautological fact contributes nothing to verification (S124)", diag);
    }
  }
  return st;
}

/* ---- Dependency-closed Datalog evaluation for one unit ---- */

/* The fact set the door loads for one unit: its dependency-closed rows (the callee summary rows it reads) */
static Status unit_fact_set(const StigiImage *img, uint32_t uidx, bool kernel_mode,
                            StigiFactSet *fs, Diag *diag) {
  Status st = stigi_extract_unit_closed(img, uidx, kernel_mode, fs, diag);
  if (st != STIGI_OK) return st;
  /* For Test units, seed testGot/testWant so tests.dl checks test-leaks-capability cleanly */
  if (img->units[uidx].ast->decl->kind == STIGI_DECL_TEST) {
    const char *got[2] = {img->units[uidx].name, "ok"};
    st = stigi_facts_add_sym(fs, "testGot", 2, got, diag);
    if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "testWant", 2, got, diag);
  }
  return st;
}

/* S134 e: from the evaluated session, the decision rows of one unit: a masked flag per element access row of the
 * unit's fact set (membership of the row's operation in `maskIndex`) and the counts of the other decision
 * relations. A unit with no element access and no allocation has none (has_dec stays false). */
static Status unit_decision(StigiDl *dl, const StigiFactSet *fs, StigiUnitDecision *dec, bool *has_dec,
                            Diag *diag) {
  *has_dec = false;
  bool has_alloc = false;
  uint32_t accesses = 0;
  for (uint32_t r = 0; r < fs->count; r++) {
    const char *rn = fs->rows[r].rel;
    if (strcmp(rn, "indexRead") == 0 || strcmp(rn, "indexWrite") == 0) accesses++;
    else if (strcmp(rn, "alloc") == 0) has_alloc = true;
  }
  dec->tracked = (accesses > 0 || has_alloc) ? 1U : 0U;
  if (!dec->tracked) { /* a record anyway: a missing record is an error in the emitter, never "no masks" */
    *has_dec = true;
    return STIGI_OK;
  }

  int rel_mask = stigi_dl_rel_id(dl, "maskIndex");
  uint32_t mask_count = rel_mask >= 0 ? stigi_dl_count(dl, rel_mask) : 0U;
  const char **mask_ops = mask_count > 0 ? malloc((size_t)mask_count * sizeof *mask_ops) : NULL;
  uint32_t nmask = 0;
  if (mask_count > 0 && mask_ops == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "mask ops OOM");
  for (uint32_t r = 0; r < mask_count; r++) {
    int64_t row[1] = {0};
    Status st = stigi_dl_read(dl, rel_mask, r, row, 1, diag);
    if (st != STIGI_OK) { free(mask_ops); return st; }
    size_t slen = 0;
    const char *s = stigi_dl_sym_str(dl, (uint32_t)row[0], &slen);
    if (s != NULL) mask_ops[nmask++] = s;
  }
  dec->access_count = accesses;
  dec->masked = accesses > 0 ? calloc(accesses, 1U) : NULL;
  if (accesses > 0 && dec->masked == NULL) {
    free(mask_ops);
    return diag_set(diag, STIGI_ERR_OOM, -1, "masked flags OOM");
  }
  uint32_t ai = 0;
  for (uint32_t r = 0; r < fs->count; r++) {
    const StigiFactRow *fr = &fs->rows[r];
    if (strcmp(fr->rel, "indexRead") != 0 && strcmp(fr->rel, "indexWrite") != 0) continue;
    bool masked = false;
    if (fr->ncols >= 1 && !fr->cols[0].is_num && fr->cols[0].sym != NULL) {
      for (uint32_t m = 0; m < nmask && !masked; m++) masked = strcmp(mask_ops[m], fr->cols[0].sym) == 0;
    }
    dec->masked[ai++] = masked ? 1U : 0U;
  }
  free(mask_ops);
  int rel_fe = stigi_dl_rel_id(dl, "freeEdge");
  int rel_fae = stigi_dl_rel_id(dl, "freeAtExit");
  int rel_ru = stigi_dl_rel_id(dl, "reuseAt");
  int rel_so = stigi_dl_rel_id(dl, "stackObject");
  dec->free_edges = rel_fe >= 0 ? stigi_dl_count(dl, rel_fe) : 0U;
  dec->free_at_exit = rel_fae >= 0 ? stigi_dl_count(dl, rel_fae) : 0U;
  dec->reuses = rel_ru >= 0 ? stigi_dl_count(dl, rel_ru) : 0U;
  dec->stack_objects = rel_so >= 0 ? stigi_dl_count(dl, rel_so) : 0U;
  *has_dec = true;
  return STIGI_OK;
}

static Status eval_unit_datalog(StigiDl *dl, bool kernel_mode, const StigiImage *img,
                                uint32_t uidx, StigiDoorResult *out, StigiUnitDecision *dec,
                                bool *has_dec, Diag *diag) {
  if (has_dec != NULL) *has_dec = false;
  Status st = stigi_dl_reset(dl, diag);
  if (st != STIGI_OK) return st;

  StigiFactSet fs;
  stigi_facts_init(&fs);
  st = unit_fact_set(img, uidx, kernel_mode, &fs, diag);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    return st;
  }

  Diag ld;
  diag_clear(&ld);
  st = stigi_facts_load_into_dl(&fs, dl, &ld);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    if (st == STIGI_ERR_DATALOG) {
      return add_reject(out, img->units[uidx].name, tupu_code_to_stigi(ld.dl_code), ld.msg, diag);
    }
    if (diag != NULL) *diag = ld;
    return st;
  }

  Diag ed;
  diag_clear(&ed);
  st = stigi_dl_eval(dl, &ed);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    return add_reject(out, img->units[uidx].name, tupu_code_to_stigi(ed.dl_code), ed.msg, diag);
  }

  if (dec != NULL && has_dec != NULL) {
    st = unit_decision(dl, &fs, dec, has_dec, diag);
    if (st != STIGI_OK) {
      stigi_facts_free(&fs);
      return st;
    }
  }
  stigi_facts_free(&fs);

  int rel_reject = stigi_dl_rel_id(dl, "reject");
  if (rel_reject >= 0) {
    uint32_t nrej = stigi_dl_count(dl, rel_reject);
    for (uint32_t r = 0; r < nrej; r++) {
      int64_t row[2] = {0, 0};
      st = stigi_dl_read(dl, rel_reject, r, row, 2, diag);
      if (st != STIGI_OK) return st;
      size_t slen = 0, clen = 0;
      const char *subj = stigi_dl_sym_str(dl, (uint32_t)row[0], &slen);
      const char *code = stigi_dl_sym_str(dl, (uint32_t)row[1], &clen);
      char sbuf[128], cbuf[64];
      (void)snprintf(sbuf, sizeof sbuf, "%.*s", (int)slen, subj != NULL ? subj : "");
      (void)snprintf(cbuf, sizeof cbuf, "%.*s", (int)clen, code != NULL ? code : "");
      st = add_reject(out, sbuf, cbuf, img->units[uidx].name, diag);
      if (st != STIGI_OK) {
        if (has_dec != NULL && *has_dec) { free(dec->masked); dec->masked = NULL; *has_dec = false; }
        return st;
      }
    }
  }
  return STIGI_OK;
}

/* ---- Export: one fact directory per door evaluation, named by the hex unit id (S134 2) ---- */

typedef struct {
  const char *root;
  uint32_t unit_dirs;
  uint32_t negtest_dirs;
  uint32_t negtest_local_dirs;
  uint32_t negtests_without_datalog;
  uint64_t rows;
} ExportCtx;

/* Create `parent/<hex id>`. Units with one id are one checked unit (identity.md, S134 b): the first one gets
 * the directory, a later one with the same id finds it and is not written again (`created` is false). */
static Status make_id_dir(const char *parent, const uint8_t id[32], char *dir, size_t cap,
                          bool *created, Diag *diag) {
  int n = snprintf(dir, cap, "%s/", parent);
  if (n < 0 || (size_t)n + 65U >= cap) {
    return diag_set(diag, STIGI_ERR_IO, -1, "export path too long");
  }
  for (int i = 0; i < 32; i++) {
    (void)snprintf(dir + n + 2 * i, 3, "%02x", (unsigned)id[i]);
  }
  *created = mkdir(dir, 0755) == 0;
  if (!*created && errno != EEXIST) {
    return diag_set(diag, STIGI_ERR_IO, errno, "cannot create %s", dir);
  }
  return STIGI_OK;
}

/* Returns the number of directories written through `wrote` (0 when the id was exported already) */
static Status write_fact_dir(ExportCtx *ex, const char *parent, const uint8_t id[32],
                             const StigiFactSet *fs, const char *rules_src, bool *wrote, Diag *diag) {
  char dir[1024];
  Status st = make_id_dir(parent, id, dir, sizeof dir, wrote, diag);
  if (st != STIGI_OK || !*wrote) return st;
  ex->rows += fs->count;
  return stigi_facts_write_dir(fs, dir, rules_src, diag);
}

/* A NegTest body as the door evaluates it: one directory `NegTest hex/local unit hex` per local unit */
static Status export_negtest_facts(StigiDoor *door, ExportCtx *ex, const char *rules_src,
                                   const StigiImage *priv, uint32_t local_start,
                                   const uint8_t nt_id[32], Diag *diag) {
  char nt_dir[1024];
  bool nt_created = false;
  Status st = make_id_dir(ex->root, nt_id, nt_dir, sizeof nt_dir, &nt_created, diag);
  for (uint32_t i = local_start; i < priv->unit_count && st == STIGI_OK; i++) {
    StigiFactSet fs;
    stigi_facts_init(&fs);
    st = unit_fact_set(priv, i, door->kernel_mode, &fs, diag);
    bool wrote = false;
    if (st == STIGI_OK) st = write_fact_dir(ex, nt_dir, priv->units[i].unit_id, &fs, rules_src, &wrote, diag);
    if (st == STIGI_OK && wrote) ex->negtest_local_dirs++;
    stigi_facts_free(&fs);
  }
  if (st == STIGI_OK && nt_created) ex->negtest_dirs++;
  return st;
}

/* ---- NegTest verifier (dynamic expect codes, known Stigi.Code.*, full pipeline) ---- */

static Status check_negtest_unit(StigiDoor *door, StigiDl *dl, const char *active_rules_dl,
                                 const StigiImage *cand, uint32_t nt_idx,
                                 StigiDoorResult *out, ExportCtx *ex, Diag *diag) {
  const StigiImageUnit *nt_u = &cand->units[nt_idx];
  const StigiDecl *nt_d = nt_u->ast->decl;
  const StigiExpr *body = nt_d->u.val_decl.body;
  if (body == NULL || body->kind != STIGI_EXPR_BLOCK) {
    return add_reject(out, nt_u->name, "negtest-form", "NegTest body must be a block", diag);
  }

  char (*expected_codes)[64] = NULL;
  uint32_t exp_count = 0, exp_cap = 0;

  StigiImage priv;
  stigi_image_init(&priv);
  StigiNameDiags pnd;
  stigi_name_diags_init(&pnd);

  Status st = STIGI_OK;
  for (uint32_t i = 0; i < cand->unit_count && st == STIGI_OK; i++) {
    if (cand->units[i].ast->decl->kind == STIGI_DECL_NEG_TEST) continue;
    Arena ta;
    arena_init(&ta);
    char *pr = NULL;
    size_t plen = 0;
    st = stigi_print_unit(cand->units[i].ast, &pr, &plen, &ta, diag);
    if (st == STIGI_OK) {
      st = stigi_image_add_source(&priv, pr, plen, &pnd, diag);
    }
    arena_free(&ta);
  }

  uint32_t local_start = priv.unit_count;
  for (uint32_t i = 0; i < body->u.block.count && st == STIGI_OK; i++) {
    const StigiExpr *s = body->u.block.stmts[i];
    if (s->kind == STIGI_EXPR_LOCAL_DECL) {
      StigiUnit tmp_u;
      memset(&tmp_u, 0, sizeof tmp_u);
      tmp_u.has_prompt = true;
      tmp_u.prompt.ptr = "negtest local.";
      tmp_u.prompt.len = 14;
      tmp_u.decl = s->u.local_decl.decl;
      Arena ta;
      arena_init(&ta);
      char *pr = NULL;
      size_t plen = 0;
      st = stigi_print_unit(&tmp_u, &pr, &plen, &ta, diag);
      if (st == STIGI_OK) {
        st = stigi_image_add_source(&priv, pr, plen, &pnd, diag);
      }
      arena_free(&ta);
    } else if (s->kind == STIGI_EXPR_CALL &&
               s->u.call.callee->kind == STIGI_EXPR_GLOBAL &&
               s->u.call.callee->u.ident.name.len == 20 &&
               memcmp(s->u.call.callee->u.ident.name.ptr, "Stigi.NegTest.expect", 20) == 0) {
      if (s->u.call.arg_count != 2 ||
          (s->u.call.args[0]->kind != STIGI_EXPR_GLOBAL &&
           s->u.call.args[0]->kind != STIGI_EXPR_LOCAL)) {
        free(expected_codes);
        stigi_name_diags_free(&pnd);
        stigi_image_free(&priv);
        return add_reject(out, nt_u->name, "negtest-form", "invalid Stigi.NegTest.expect form", diag);
      }
      StigiStr code_sym = s->u.call.args[0]->u.ident.name;
      if (code_sym.len < 11 || memcmp(code_sym.ptr, "Stigi.Code.", 11) != 0 ||
          !stigi_is_known_code_case(code_sym.ptr + 11, code_sym.len - 11)) {
        free(expected_codes);
        stigi_name_diags_free(&pnd);
        stigi_image_free(&priv);
        return add_reject(out, nt_u->name, "negtest-form", "unknown Stigi.Code case", diag);
      }
      char kebab[64];
      code_to_kebab(code_sym, kebab, sizeof kebab);
      for (uint32_t e = 0; e < exp_count; e++) {
        if (strcmp(expected_codes[e], kebab) == 0) {
          free(expected_codes);
          stigi_name_diags_free(&pnd);
          stigi_image_free(&priv);
          return add_reject(out, nt_u->name, "negtest-form", "duplicate expected code in NegTest", diag);
        }
      }
      if (exp_count == exp_cap) {
        uint32_t ncap = exp_cap == 0 ? 4U : exp_cap * 2U;
        char (*nc)[64] = realloc(expected_codes, (size_t)ncap * sizeof *nc);
        if (nc == NULL) {
          free(expected_codes);
          stigi_name_diags_free(&pnd);
          stigi_image_free(&priv);
          return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        }
        expected_codes = nc;
        exp_cap = ncap;
      }
      memcpy(expected_codes[exp_count++], kebab, sizeof kebab);
    }
  }

  if (st != STIGI_OK) {
    free(expected_codes);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return st;
  }

  if (exp_count == 0) {
    free(expected_codes);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return add_reject(out, nt_u->name, "negtest-form", "NegTest missing expect()", diag);
  }

  /* Verify every expected code has a real clearing rule (T2: negtest-without-rule) */
  for (uint32_t e = 0; e < exp_count; e++) {
    if (!code_has_clearing_rule_in(active_rules_dl, expected_codes[e])) {
      char bad_code[64];
      memcpy(bad_code, expected_codes[e], sizeof bad_code);
      free(expected_codes);
      stigi_name_diags_free(&pnd);
      stigi_image_free(&priv);
      return add_reject(out, nt_u->name, "negtest-without-rule", bad_code, diag);
    }
  }

  for (uint32_t i = local_start; i < priv.unit_count; i++) {
    priv.units[i].is_root = true;
  }

  StigiDoorResult priv_res;
  stigi_door_result_init(&priv_res);

  st = stigi_image_resolve(&priv, &pnd, diag);
  if (st != STIGI_OK) {
    stigi_door_result_free(&priv_res);
    free(expected_codes);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return st;
  }
  for (uint32_t i = 0; i < pnd.count && st == STIGI_OK; i++) {
    if (pnd.items[i].code == STIGI_NAME_NO_READER || pnd.items[i].code == STIGI_NAME_PUB_UNUSED) {
      continue;
    }
    st = add_reject(&priv_res, nt_u->name, stigi_name_code_name(pnd.items[i].code),
                    pnd.items[i].msg, diag);
  }

  if (st == STIGI_OK) {
    st = check_boundary_rules(&priv, door->kernel_mode, &priv_res, diag);
  }

  if (st == STIGI_OK) {
    StigiTypeDiags ptd;
    stigi_type_diags_init(&ptd);
    st = stigi_typecheck_image(&priv, &ptd, diag);
    for (uint32_t i = 0; i < ptd.count && st == STIGI_OK; i++) {
      st = add_reject(&priv_res, nt_u->name, stigi_type_code_name(ptd.items[i].code),
                      ptd.items[i].msg, diag);
    }
    stigi_type_diags_free(&ptd);
  }

  /* Compile any custom rules declared inside the NegTest body */
  StigiDl *priv_dl = dl;
  StigiDl local_dl;
  memset(&local_dl, 0, sizeof local_dl);
  char *priv_dl_src = NULL;
  size_t priv_dl_len = 0;
  bool priv_has_custom = false;
  if (st == STIGI_OK && priv_res.reject_count == 0) {
    st = stigi_door_compile_rules(door, &priv, &priv_dl_src, &priv_dl_len, &priv_has_custom, diag);
    if (st == STIGI_OK && priv_has_custom) {
      Diag od;
      diag_clear(&od);
      if (stigi_dl_open(&local_dl, priv_dl_src, priv_dl_len, "negtest.dl",
                        door->row_budget, 1, &od) != STIGI_OK) {
        st = add_reject(&priv_res, nt_u->name, tupu_code_to_stigi(od.dl_code), od.msg, diag);
      } else {
        priv_dl = &local_dl;
      }
    }
  }

  if (st == STIGI_OK && priv_res.reject_count == 0) {
    if (ex != NULL) {
      st = export_negtest_facts(door, ex, priv_has_custom ? priv_dl_src : active_rules_dl, &priv,
                                local_start, nt_u->unit_id, diag);
    } else {
      for (uint32_t i = local_start; i < priv.unit_count && st == STIGI_OK; i++) {
        st = eval_unit_datalog(priv_dl, door->kernel_mode, &priv, i, &priv_res, NULL, NULL, diag);
      }
    }
  } else if (st == STIGI_OK && ex != NULL) {
    ex->negtests_without_datalog++;
  }

  stigi_dl_close(&local_dl);
  free(priv_dl_src);
  stigi_name_diags_free(&pnd);
  stigi_image_free(&priv);

  if (st != STIGI_OK) {
    stigi_door_result_free(&priv_res);
    free(expected_codes);
    return st;
  }

  if (priv_res.reject_count == 0) {
    stigi_door_result_free(&priv_res);
    free(expected_codes);
    return add_reject(out, nt_u->name, "negtest-not-rejected",
                      "NegTest body was accepted instead of rejected", diag);
  }

  /* Exact set equality expected == actual AND exactly 1 subject per expected code */
  for (uint32_t e = 0; e < exp_count; e++) {
    uint32_t hits = 0;
    for (uint32_t r = 0; r < priv_res.reject_count; r++) {
      if (strcmp(priv_res.rejects[r].code, expected_codes[e]) == 0) hits++;
    }
    if (hits == 0) {
      char msg[256];
      (void)snprintf(msg, sizeof msg, "NegTest expected '%.60s', got '%.60s'",
                     expected_codes[e], priv_res.rejects[0].code);
      stigi_door_result_free(&priv_res);
      free(expected_codes);
      return add_reject(out, nt_u->name, "negtest-wrong-code", msg, diag);
    }
    if (hits > 1) {
      stigi_door_result_free(&priv_res);
      free(expected_codes);
      return add_reject(out, nt_u->name, "negtest-form",
                        "NegTest expected code matched multiple subjects", diag);
    }
  }
  for (uint32_t r = 0; r < priv_res.reject_count; r++) {
    bool in_exp = false;
    for (uint32_t e = 0; e < exp_count; e++) {
      if (strcmp(priv_res.rejects[r].code, expected_codes[e]) == 0) {
        in_exp = true;
        break;
      }
    }
    if (!in_exp) {
      char msg[256];
      (void)snprintf(msg, sizeof msg, "NegTest produced unexpected code '%.60s'",
                     priv_res.rejects[r].code);
      stigi_door_result_free(&priv_res);
      free(expected_codes);
      return add_reject(out, nt_u->name, "negtest-wrong-code", msg, diag);
    }
  }

  stigi_door_result_free(&priv_res);
  free(expected_codes);
  return STIGI_OK;
}

/* Lock-free embarrassingly parallel unit verdict worker (T18: byte-identical to sequential) */
typedef struct {
  uint32_t unit_idx;
  uint8_t vkey[32];
  StigiDoorResult res;
  Status st;
  Diag diag;
  StigiUnitDecision dec; /* S134 e: the unit's decision rows when it has element accesses or allocations */
  bool has_dec;
} ParallelUnitTask;

typedef struct {
  const char *rules_src;
  size_t rules_len;
  uint64_t row_budget;
  bool kernel_mode;
  const StigiImage *cand;
  ParallelUnitTask *tasks;
  uint32_t task_count;
  uint32_t worker_id;
  uint32_t worker_count;
  StigiDl *shared_dl; /* non-NULL only on worker 0 */
} ParallelWorkerArg;

static void *parallel_verdict_worker(void *arg_p) {
  ParallelWorkerArg *w = (ParallelWorkerArg *)arg_p;
  StigiDl local_dl;
  memset(&local_dl, 0, sizeof local_dl);
  StigiDl *dl = w->shared_dl;
  if (dl == NULL) {
    Diag od;
    diag_clear(&od);
    Status ost = stigi_dl_open(&local_dl, w->rules_src, w->rules_len, "parallel.dl",
                               w->row_budget, 1, &od);
    if (ost != STIGI_OK) {
      for (uint32_t k = w->worker_id; k < w->task_count; k += w->worker_count) {
        w->tasks[k].st = ost;
        w->tasks[k].diag = od;
      }
      return NULL;
    }
    dl = &local_dl;
  }
  for (uint32_t k = w->worker_id; k < w->task_count; k += w->worker_count) {
    ParallelUnitTask *t = &w->tasks[k];
    diag_clear(&t->diag);
    t->st = eval_unit_datalog(dl, w->kernel_mode, w->cand, t->unit_idx, &t->res, &t->dec, &t->has_dec, &t->diag);
  }
  if (w->shared_dl == NULL) {
    stigi_dl_close(&local_dl);
  }
  return NULL;
}

static uint32_t choose_door_threads(uint32_t task_count) {
  if (task_count < 4U) return 1U;
  const char *env = getenv("STIGI_THREADS");
  if (env != NULL && env[0] != '\0') {
    long v = strtol(env, NULL, 10);
    if (v <= 1) return 1U;
    uint32_t req = v > 16 ? 16U : (uint32_t)v;
    return req > task_count ? task_count : req;
  }
  long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
  if (ncpu <= 1) return 1U;
  uint32_t nt = ncpu > 8 ? 8U : (uint32_t)ncpu;
  uint32_t max_useful = task_count / 2U;
  if (nt > max_useful) nt = max_useful;
  return nt < 1U ? 1U : nt;
}

/* ---- Atomic transaction pipeline ---- */

static Status run_transaction(StigiDoor *door, const char *const *add_srcs,
                              const size_t *add_lens, uint32_t add_count,
                              const char *const *drop_names, uint32_t drop_count,
                              bool replace_all, bool commit, StigiDoorResult *out, Diag *diag) {
  if (door == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "run_transaction: invalid arguments");
  }
  out->accepted = false;

  /* Step 1: Single-pass Lex -> Tenet Gate + Parse on all incoming units */
  char **new_names = NULL;
  if (add_count > 0) {
    new_names = calloc(add_count, sizeof *new_names);
    if (new_names == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }

  Status tx_st = STIGI_OK;
  for (uint32_t i = 0; i < add_count && tx_st == STIGI_OK; i++) {
    Arena pa;
    arena_init(&pa);
    StigiTok *toks = NULL;
    uint32_t tok_count = 0;
    tx_st = stigi_lex_all(add_srcs[i], add_lens[i], &toks, &tok_count, &pa, diag);
    if (tx_st != STIGI_OK) {
      arena_free(&pa);
      break;
    }

    StigiGateFindings gf;
    stigi_gate_findings_init(&gf);
    tx_st = stigi_gate_check_tokens(toks, tok_count, &gf, diag);
    if (tx_st == STIGI_OK) {
      for (uint32_t k = 0; k < gf.count && tx_st == STIGI_OK; k++) {
        tx_st = add_reject(out, "<input>", stigi_gate_code_name(gf.items[k].code),
                           gf.items[k].msg, diag);
      }
    }
    stigi_gate_findings_free(&gf);
    if (tx_st != STIGI_OK) {
      arena_free(&pa);
      break;
    }

    StigiParseResult pr = stigi_parse_tokens(toks, tok_count, &pa);
    if (pr.status != STIGI_OK || pr.unit == NULL || pr.unit->decl == NULL) {
      tx_st = add_reject(out, "<input>", pr.diag_code != NULL ? pr.diag_code : "parse",
                         pr.msg, diag);
    } else {
      StigiStr nm = pr.unit->decl->name;
      new_names[i] = malloc(nm.len + 1U);
      if (new_names[i] == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      } else {
        memcpy(new_names[i], nm.ptr, nm.len);
        new_names[i][nm.len] = '\0';
      }
    }
    arena_free(&pa);
  }

  /* Verify all dropped names exist in current image */
  for (uint32_t d = 0; !replace_all && d < drop_count && tx_st == STIGI_OK; d++) {
    if (drop_names[d] == NULL ||
        stigi_image_find(&door->image, drop_names[d], strlen(drop_names[d])) < 0) {
      tx_st = add_reject(out, drop_names[d] != NULL ? drop_names[d] : "<drop>",
                         "unknown-unit", "cannot drop unit not in image", diag);
    }
  }

  if (tx_st != STIGI_OK || out->reject_count > 0) {
    for (uint32_t i = 0; i < add_count; i++) free(new_names[i]);
    free(new_names);
    return tx_st;
  }

  /* Build candidate source list */
  uint32_t max_srcs = door->src_count + add_count;
  char **cand_srcs = calloc(max_srcs > 0 ? max_srcs : 1U, sizeof *cand_srcs);
  size_t *cand_lens = calloc(max_srcs > 0 ? max_srcs : 1U, sizeof *cand_lens);
  if (cand_srcs == NULL || cand_lens == NULL) {
    free(cand_srcs);
    free(cand_lens);
    for (uint32_t i = 0; i < add_count; i++) free(new_names[i]);
    free(new_names);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  uint32_t cand_count = 0;

  for (uint32_t i = 0; !replace_all && i < door->src_count && tx_st == STIGI_OK; i++) {
    const char *uname = i < door->image.unit_count ? door->image.units[i].name : "";
    bool skip = false;
    for (uint32_t d = 0; d < drop_count; d++) {
      if (strcmp(uname, drop_names[d]) == 0) { skip = true; break; }
    }
    for (uint32_t a = 0; !skip && a < add_count; a++) {
      if (new_names[a] != NULL && strcmp(uname, new_names[a]) == 0) { skip = true; break; }
    }
    if (!skip) {
      cand_srcs[cand_count] = malloc(door->unit_lens[i] + 1U);
      if (cand_srcs[cand_count] == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        break;
      }
      memcpy(cand_srcs[cand_count], door->unit_srcs[i], door->unit_lens[i] + 1U);
      cand_lens[cand_count] = door->unit_lens[i];
      cand_count++;
    }
  }
  for (uint32_t i = 0; i < add_count; i++) {
    if (tx_st == STIGI_OK) {
      cand_srcs[cand_count] = malloc(add_lens[i] + 1U);
      if (cand_srcs[cand_count] == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      } else {
        memcpy(cand_srcs[cand_count], add_srcs[i], add_lens[i]);
        cand_srcs[cand_count][add_lens[i]] = '\0';
        cand_lens[cand_count] = add_lens[i];
        cand_count++;
      }
    }
    free(new_names[i]);
  }
  free(new_names);

  /* Door.md 106: on pure drop, tree-shake units no remaining root reaches */
  if (tx_st == STIGI_OK && drop_count > 0 && add_count == 0 && !replace_all && cand_count > 0) {
    StigiImage tmp_img;
    stigi_image_init(&tmp_img);
    StigiNameDiags tmp_nd;
    stigi_name_diags_init(&tmp_nd);
    bool ok = true;
    for (uint32_t i = 0; i < cand_count && ok; i++) {
      if (stigi_image_add_source(&tmp_img, cand_srcs[i], cand_lens[i], &tmp_nd, diag) != STIGI_OK) {
        ok = false;
      }
    }
    if (ok && stigi_image_resolve(&tmp_img, &tmp_nd, diag) == STIGI_OK) {
      /* Only prune if there were no unresolved references from remaining roots */
      bool has_unres = false;
      for (uint32_t i = 0; i < tmp_nd.count; i++) {
        if (tmp_nd.items[i].code == STIGI_NAME_UNRESOLVED) { has_unres = true; break; }
      }
      if (!has_unres && tmp_img.unit_count == cand_count) {
        uint32_t w = 0;
        for (uint32_t i = 0; i < cand_count; i++) {
          if (tmp_img.units[i].reachable) {
            cand_srcs[w] = cand_srcs[i];
            cand_lens[w] = cand_lens[i];
            w++;
          } else {
            free(cand_srcs[i]);
          }
        }
        cand_count = w;
      }
    }
    stigi_name_diags_free(&tmp_nd);
    stigi_image_free(&tmp_img);
  }

  /* Stage cache updates locally; merge ONLY on commit */
  StigiKeySet staged_vkeys = {NULL, 0, 0};
  StigiKeySet staged_tkeys = {NULL, 0, 0};
  StigiDecisionStore staged_decs = {NULL, 0, 0};

  /* Step 2: Names + unit_id / verdict_key (C3, S119) + boundary checks */
  StigiImage cand;
  stigi_image_init(&cand);
  StigiNameDiags nd;
  stigi_name_diags_init(&nd);
  for (uint32_t i = 0; i < cand_count && tx_st == STIGI_OK; i++) {
    Status st = stigi_image_add_source(&cand, cand_srcs[i], cand_lens[i], &nd, diag);
    if (st != STIGI_OK) {
      if (st == STIGI_ERR_OOM) tx_st = st;
      else tx_st = add_reject(out, "<input>", "parse", diag->msg, diag);
    }
  }
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    tx_st = stigi_image_resolve(&cand, &nd, diag);
  }
  for (uint32_t i = 0; i < nd.count && tx_st == STIGI_OK; i++) {
    tx_st = add_reject(out, "<names>", stigi_name_code_name(nd.items[i].code),
                       nd.items[i].msg, diag);
  }
  stigi_name_diags_free(&nd);

  if (tx_st == STIGI_OK) {
    tx_st = check_boundary_rules(&cand, door->kernel_mode, out, diag);
  }

  /* Step 3: Types (C4) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    StigiTypeDiags td;
    stigi_type_diags_init(&td);
    tx_st = stigi_typecheck_image(&cand, &td, diag);
    for (uint32_t i = 0; i < td.count && tx_st == STIGI_OK; i++) {
      tx_st = add_reject(out, "<types>", stigi_type_code_name(td.items[i].code),
                         td.items[i].msg, diag);
    }
    stigi_type_diags_free(&td);
  }

  /* Step 3b: Compile in-image rules & relations into active candidate StigiDl */
  StigiDl *eval_dl = &door->dl;
  StigiDl cand_dl;
  memset(&cand_dl, 0, sizeof cand_dl);
  char *cand_rules_dl = NULL;
  size_t cand_rules_len = 0;
  bool has_custom_rules = false;
  bool need_dl_swap = false;

  if (tx_st == STIGI_OK && out->reject_count == 0) {
    tx_st = stigi_door_compile_rules(door, &cand, &cand_rules_dl, &cand_rules_len,
                                &has_custom_rules, diag);
    if (tx_st == STIGI_OK && has_custom_rules) {
      Diag od;
      diag_clear(&od);
      if (stigi_dl_open(&cand_dl, cand_rules_dl, cand_rules_len, "image.dl",
                        door->row_budget, 1, &od) != STIGI_OK) {
        tx_st = add_reject(out, "<rules>", tupu_code_to_stigi(od.dl_code), od.msg, diag);
      } else {
        eval_dl = &cand_dl;
        need_dl_swap = true;
      }
    } else if (tx_st == STIGI_OK && !has_custom_rules &&
               door->rules_dl_len != door->base_rules_dl_len) {
      /* All custom rules were dropped; rebuild clean base_rules_dl session */
      cand_rules_dl = malloc(door->base_rules_dl_len + 1U);
      if (cand_rules_dl == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      } else {
        memcpy(cand_rules_dl, door->base_rules_dl, door->base_rules_dl_len + 1U);
        cand_rules_len = door->base_rules_dl_len;
        tx_st = stigi_dl_open(&cand_dl, cand_rules_dl, cand_rules_len, "all.dl",
                              door->row_budget, 1, diag);
        if (tx_st == STIGI_OK) {
          eval_dl = &cand_dl;
          need_dl_swap = true;
        }
      }
    }
  }

  const char *active_rules_src = cand_rules_dl != NULL ? cand_rules_dl : door->rules_dl;
  uint8_t rules_hash[32];
  memset(rules_hash, 0, sizeof rules_hash);
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    tx_st = compute_active_rules_hash(door, &cand, rules_hash, diag);
  }

  /* Steps 4 & 5: Extract dependency-closed facts + run Datalog per unit (lock-free parallel T18!) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    uint32_t todo_cnt = 0;
    ParallelUnitTask *tasks = cand.unit_count > 0
                                  ? calloc(cand.unit_count, sizeof *tasks)
                                  : NULL;
    if (cand.unit_count > 0 && tasks == NULL) {
      tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    } else {
      /* Units with one id are one checked unit (identity.md, S134 b): the door evaluates one id once */
      StigiKeySet queued = {NULL, 0, 0};
      for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
        StigiDeclKind k = cand.units[i].ast->decl->kind;
        if (k == STIGI_DECL_NEG_TEST) continue;
        uint8_t vkey[32];
        effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->seed_id, door->row_budget,
                              door->kernel_mode, vkey);
        if (keyset_contains(&door->verdict_cache, vkey)) {
          out->verdicts_cached++;
          continue;
        }
        if (keyset_contains(&queued, vkey)) continue;
        tx_st = keyset_insert(&queued, vkey, diag);
        if (tx_st != STIGI_OK) break;
        out->verdicts_computed++;
        tasks[todo_cnt].unit_idx = i;
        memcpy(tasks[todo_cnt].vkey, vkey, 32U);
        stigi_door_result_init(&tasks[todo_cnt].res);
        tasks[todo_cnt].st = STIGI_OK;
        todo_cnt++;
      }
      keyset_free(&queued);

      size_t active_rules_len = cand_rules_dl != NULL ? cand_rules_len : door->rules_dl_len;
      uint32_t nthreads = choose_door_threads(todo_cnt);
      if (nthreads <= 1U) {
        ParallelWorkerArg w0 = {
            active_rules_src, active_rules_len, door->row_budget, door->kernel_mode,
            &cand, tasks, todo_cnt, 0U, 1U, eval_dl
        };
        (void)parallel_verdict_worker(&w0);
      } else {
        ParallelWorkerArg *wargs = calloc(nthreads, sizeof *wargs);
        pthread_t *tids = calloc(nthreads, sizeof *tids);
        if (wargs == NULL || tids == NULL) {
          free(wargs);
          free(tids);
          ParallelWorkerArg w0 = {
              active_rules_src, active_rules_len, door->row_budget, door->kernel_mode,
              &cand, tasks, todo_cnt, 0U, 1U, eval_dl
          };
          (void)parallel_verdict_worker(&w0);
        } else {
          uint32_t spawned = 0;
          for (uint32_t t = 0; t < nthreads; t++) {
            wargs[t].rules_src = active_rules_src;
            wargs[t].rules_len = active_rules_len;
            wargs[t].row_budget = door->row_budget;
            wargs[t].kernel_mode = door->kernel_mode;
            wargs[t].cand = &cand;
            wargs[t].tasks = tasks;
            wargs[t].task_count = todo_cnt;
            wargs[t].worker_id = t;
            wargs[t].worker_count = nthreads;
            wargs[t].shared_dl = (t == 0U) ? eval_dl : NULL;
          }
          for (uint32_t t = 1U; t < nthreads; t++) {
            if (pthread_create(&tids[t], NULL, parallel_verdict_worker, &wargs[t]) != 0) {
              /* Fallback: run remaining shards synchronously on main thread */
              (void)parallel_verdict_worker(&wargs[t]);
              tids[t] = 0;
            } else {
              spawned++;
            }
          }
          (void)parallel_verdict_worker(&wargs[0]);
          for (uint32_t t = 1U; t < nthreads; t++) {
            if (tids[t] != 0) (void)pthread_join(tids[t], NULL);
          }
          (void)spawned;
          free(wargs);
          free(tids);
        }
      }

      /* Deterministic merge strictly in original unit order 0..todo_cnt-1 (T18 Seq oracle) */
      for (uint32_t k = 0; k < todo_cnt; k++) {
        if (tx_st == STIGI_OK && tasks[k].st != STIGI_OK) {
          tx_st = tasks[k].st;
          if (diag != NULL) *diag = tasks[k].diag;
        }
        if (tx_st == STIGI_OK) {
          if (tasks[k].res.reject_count == 0) {
            tx_st = keyset_insert(&staged_vkeys, tasks[k].vkey, diag);
            if (tx_st == STIGI_OK && tasks[k].has_dec) {
              tx_st = stigi_decisions_insert(&staged_decs, tasks[k].vkey, &tasks[k].dec, diag);
            }
          } else {
            for (uint32_t r = 0; r < tasks[k].res.reject_count && tx_st == STIGI_OK; r++) {
              tx_st = add_reject(out, tasks[k].res.rejects[r].subject,
                                 tasks[k].res.rejects[r].code,
                                 tasks[k].res.rejects[r].msg, diag);
            }
          }
        }
        stigi_door_result_free(&tasks[k].res);
      }
      for (uint32_t k = 0; k < todo_cnt; k++) free(tasks[k].dec.masked);
      free(tasks);
    }
  }

  /* Step 6: NegTests */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      if (cand.units[i].ast->decl->kind != STIGI_DECL_NEG_TEST) continue;
      uint8_t ntkey[32];
      effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->seed_id, door->row_budget,
                            door->kernel_mode, ntkey);
      if (keyset_contains(&door->test_cache, ntkey)) {
        out->tests_cached++;
        continue;
      }
      out->tests_computed++;
      uint32_t before = out->reject_count;
      tx_st = check_negtest_unit(door, eval_dl, active_rules_src, &cand, i, out, NULL, diag);
      if (tx_st == STIGI_OK && out->reject_count == before) {
        tx_st = keyset_insert(&staged_tkeys, ntkey, diag);
      }
    }
  }

  /* Step 7: Tests & compile-time vals */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    Arena eval_arena;
    arena_init(&eval_arena);
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      StigiDeclKind k = cand.units[i].ast->decl->kind;
      if (k == STIGI_DECL_TEST) {
        uint8_t tkey[32];
        effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->seed_id, door->row_budget,
                              door->kernel_mode, tkey);
        if (keyset_contains(&door->test_cache, tkey)) {
          out->tests_cached++;
          continue;
        }
        out->tests_computed++;
        StigiEvalResult er = stigi_eval_test(&cand, i, STIGI_DEFAULT_FUEL, &eval_arena);
        if (er.status != STIGI_OK || er.value.kind != STIGI_VAL_TEST_OK) {
          tx_st = add_reject(out, cand.units[i].name,
                             er.diag_code != NULL ? er.diag_code : "test-failed",
                             er.msg[0] != '\0' ? er.msg : "Test did not return Stigi.Test.Result",
                             diag);
        } else {
          tx_st = keyset_insert(&staged_tkeys, tkey, diag);
        }
      } else if (k == STIGI_DECL_VAL) {
        StigiEvalResult er = stigi_eval_unit(&cand, i, STIGI_DEFAULT_FUEL, &eval_arena);
        if (er.status != STIGI_OK) {
          tx_st = add_reject(out, cand.units[i].name,
                             er.diag_code != NULL ? er.diag_code : "eval-error", er.msg, diag);
        }
      }
      /* the verdict is copied out (msg inline, code literal, value unread): drop this unit's eval memory */
      arena_reset(&eval_arena);
    }
    arena_free(&eval_arena);
  }

  /* Step 8: Atomic commit or rollback (pre-reserve cache capacity so commit cannot half-fail) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    if (commit) {
      tx_st = keyset_reserve(&door->verdict_cache, staged_vkeys.count, diag);
      if (tx_st == STIGI_OK) {
        tx_st = keyset_reserve(&door->test_cache, staged_tkeys.count, diag);
      }
    }
    if (commit && tx_st == STIGI_OK) {
      tx_st = decisions_reserve(&door->decisions, staged_decs.count, diag);
    }
    if (tx_st == STIGI_OK) {
      if (commit) {
        for (uint32_t i = 0; i < staged_vkeys.cap; i++) {
          if (staged_vkeys.slots[i].used) {
            (void)keyset_insert(&door->verdict_cache, staged_vkeys.slots[i].key, diag);
          }
        }
        for (uint32_t i = 0; i < staged_tkeys.cap; i++) {
          if (staged_tkeys.slots[i].used) {
            (void)keyset_insert(&door->test_cache, staged_tkeys.slots[i].key, diag);
          }
        }
        for (uint32_t i = 0; i < staged_decs.count; i++) {
          (void)stigi_decisions_insert(&door->decisions, staged_decs.recs[i].key, &staged_decs.recs[i].dec, diag);
        }
        if (need_dl_swap) {
          stigi_dl_close(&door->dl);
          door->dl = cand_dl;
          free(door->rules_dl);
          door->rules_dl = cand_rules_dl;
          door->rules_dl_len = cand_rules_len;
          cand_dl.sess = NULL;
          cand_rules_dl = NULL;
        }
        stigi_image_free(&door->image);
        door->image = cand;
        for (uint32_t i = 0; i < door->src_count; i++) free(door->unit_srcs[i]);
        free(door->unit_srcs);
        free(door->unit_lens);
        door->unit_srcs = cand_srcs;
        door->unit_lens = cand_lens;
        door->src_count = cand_count;
        door->src_cap = max_srcs;
        out->accepted = true;
        keyset_free(&staged_vkeys);
        stigi_decisions_free(&staged_decs);
        keyset_free(&staged_tkeys);
        return STIGI_OK;
      }
      out->accepted = true;
      keyset_free(&staged_vkeys);
        stigi_decisions_free(&staged_decs);
      keyset_free(&staged_tkeys);
      stigi_dl_close(&cand_dl);
      free(cand_rules_dl);
      stigi_image_free(&cand);
      for (uint32_t i = 0; i < cand_count; i++) free(cand_srcs[i]);
      free(cand_srcs);
      free(cand_lens);
      return STIGI_OK;
    }
  }

  /* Rollback: live door->image, door->dl, and door caches are 100% untouched */
  keyset_free(&staged_vkeys);
        stigi_decisions_free(&staged_decs);
  keyset_free(&staged_tkeys);
  stigi_dl_close(&cand_dl);
  free(cand_rules_dl);
  stigi_image_free(&cand);
  for (uint32_t i = 0; i < cand_count; i++) free(cand_srcs[i]);
  free(cand_srcs);
  free(cand_lens);
  return tx_st;
}

Status stigi_door_submit(StigiDoor *door, const char *const *srcs, const size_t *lens,
                         uint32_t count, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, count, NULL, 0, false, true, out, diag);
}

Status stigi_door_check(StigiDoor *door, const char *const *srcs, const size_t *lens,
                        uint32_t count, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, count, NULL, 0, false, false, out, diag);
}

Status stigi_door_drop(StigiDoor *door, const char *const *names, uint32_t count,
                       StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, NULL, NULL, 0, names, count, false, true, out, diag);
}

Status stigi_door_tx(StigiDoor *door, const char *const *srcs, const size_t *lens,
                     uint32_t src_count, const char *const *drop_names, uint32_t drop_count,
                     bool commit, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, src_count, drop_names, drop_count, false, commit,
                         out, diag);
}

Status stigi_door_replace_all(StigiDoor *door, const char *const *srcs, const size_t *lens,
                              uint32_t count, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, count, NULL, 0, true, true, out, diag);
}

Status stigi_door_cache_add_verdict(StigiDoor *door, const uint8_t key[32], Diag *diag) {
  if (door == NULL || key == NULL) return diag_set(diag, STIGI_ERR_INVALID, -1, "invalid args");
  return keyset_insert(&door->verdict_cache, key, diag);
}

Status stigi_door_cache_add_test(StigiDoor *door, const uint8_t key[32], Diag *diag) {
  if (door == NULL || key == NULL) return diag_set(diag, STIGI_ERR_INVALID, -1, "invalid args");
  return keyset_insert(&door->test_cache, key, diag);
}

Status stigi_door_set_budget(StigiDoor *door, uint64_t new_budget,
                             StigiDoorResult *out, Diag *diag) {
  if (door == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_set_budget: invalid arguments");
  }
  out->accepted = false;
  if (new_budget == 0) {
    return add_reject(out, "Stigi.Datalog.rowBudget", "datalog-budget",
                      "rowBudget must be > 0", diag);
  }

  StigiDl tmp_dl;
  Diag od;
  diag_clear(&od);
  Status st = stigi_dl_open(&tmp_dl, door->rules_dl, door->rules_dl_len, "all.dl",
                            new_budget, 1, &od);
  if (st != STIGI_OK) {
    return add_reject(out, "Stigi.Datalog.rowBudget", tupu_code_to_stigi(od.dl_code),
                      od.msg, diag);
  }

  for (uint32_t i = 0; i < door->image.unit_count && st == STIGI_OK; i++) {
    StigiDeclKind k = door->image.units[i].ast->decl->kind;
    out->verdicts_computed++;
    if (k == STIGI_DECL_NEG_TEST) {
      st = check_negtest_unit(door, &tmp_dl, door->rules_dl, &door->image, i, out, NULL, diag);
    } else {
      st = eval_unit_datalog(&tmp_dl, door->kernel_mode, &door->image, i, out, NULL, NULL, diag);
    }
  }

  if (st == STIGI_OK && out->reject_count == 0) {
    stigi_dl_close(&door->dl);
    door->dl = tmp_dl;
    door->row_budget = new_budget;
    out->accepted = true;
    return STIGI_OK;
  }

  stigi_dl_close(&tmp_dl);
  return st;
}

/* Full image fact extraction including door metadata for true C7 tupu parity */
Status stigi_door_extract_image(const StigiDoor *door, StigiFactSet *fs, Diag *diag) {
  if (door == NULL || fs == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_extract_image: invalid arguments");
  }
  Status st = stigi_extract_image(&door->image, fs, diag);
  if (st != STIGI_OK) return st;

  const char *mode_s[1] = {door->kernel_mode ? "Kernel" : "User"};
  st = stigi_facts_add_sym(fs, "imageMode", 1, mode_s, diag);
  if (st != STIGI_OK) return st;

  for (uint32_t i = 0; i < door->image.unit_count && st == STIGI_OK; i++) {
    const StigiImageUnit *u = &door->image.units[i];
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_TEST) {
      const char *tv[2] = {u->name, "ok"};
      st = stigi_facts_add_sym(fs, "testGot", 2, tv, diag);
      if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "testWant", 2, tv, diag);
    } else if (d->kind == STIGI_DECL_NEG_TEST) {
      const StigiExpr *body = d->u.val_decl.body;
      if (body != NULL && body->kind == STIGI_EXPR_BLOCK) {
        for (uint32_t k = 0; k < body->u.block.count && st == STIGI_OK; k++) {
          const StigiExpr *s = body->u.block.stmts[k];
          if (s->kind == STIGI_EXPR_CALL &&
              s->u.call.callee->kind == STIGI_EXPR_GLOBAL &&
              s->u.call.callee->u.ident.name.len == 20 &&
              memcmp(s->u.call.callee->u.ident.name.ptr, "Stigi.NegTest.expect", 20) == 0 &&
              s->u.call.arg_count == 2 &&
              (s->u.call.args[0]->kind == STIGI_EXPR_GLOBAL ||
               s->u.call.args[0]->kind == STIGI_EXPR_LOCAL)) {
            char kebab[64];
            code_to_kebab(s->u.call.args[0]->u.ident.name, kebab, sizeof kebab);
            const char *ne[2] = {u->name, kebab};
            const char *nr[3] = {u->name, "s0", kebab};
            const char *cr[2] = {"kernel", kebab};
            st = stigi_facts_add_sym(fs, "negExpect", 2, ne, diag);
            if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "negRejects", 3, nr, diag);
            if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "clearsRule", 2, cr, diag);
          }
        }
      }
    }
  }
  return st;
}

/* ---- S127 Comptime Tree-Shaking: Constant Folding, Pure Call Eval, Dead-Local & Unit GC ---- */

static StigiExpr *ts_make_u64_lit(Arena *a, uint64_t val, StigiSpan sp) {
  StigiExpr *e = arena_alloc(a, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_LIT_INT;
  e->span = sp;
  e->u.lit_int.value = val;
  char tmp[32];
  int n = snprintf(tmp, sizeof tmp, "%llu", (unsigned long long)val);
  if (n > 0) {
    char *cp = arena_alloc(a, (size_t)n + 1U);
    if (cp != NULL) {
      memcpy(cp, tmp, (size_t)n + 1U);
      e->u.lit_int.raw = (StigiStr){ cp, (uint32_t)n };
    }
  }
  return e;
}

static StigiExpr *ts_make_bool_lit(Arena *a, bool val, StigiSpan sp) {
  StigiExpr *e = arena_alloc(a, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_LIT_BOOL;
  e->span = sp;
  e->u.lit_bool.value = val;
  return e;
}

static bool ts_def_is_pure_computable(const StigiImage *img, uint32_t uidx) {
  if (uidx >= img->unit_count) return false;
  const StigiDecl *d = img->units[uidx].ast->decl;
  if (d->kind != STIGI_DECL_DEF || d->is_pub) return false;
  const StigiDefHead *h = &d->u.def_decl.head;
  if (h->type_param_count > 0) return false;
  for (uint32_t g = 0; g < h->pre_group_count; g++) {
    if (!h->pre_groups[g].is_fact && h->pre_groups[g].cap_count > 0) return false;
  }
  return true;
}

static bool ts_expr_uses_ident(const StigiExpr *e, StigiStr name) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      return e->u.ident.name.len == name.len &&
             memcmp(e->u.ident.name.ptr, name.ptr, name.len) == 0;
    case STIGI_EXPR_UNOP:
      return ts_expr_uses_ident(e->u.unop.operand, name);
    case STIGI_EXPR_BINOP:
      return ts_expr_uses_ident(e->u.binop.lhs, name) ||
             ts_expr_uses_ident(e->u.binop.rhs, name);
    case STIGI_EXPR_IF:
      return ts_expr_uses_ident(e->u.if_expr.cond, name) ||
             ts_expr_uses_ident(e->u.if_expr.then_br, name) ||
             ts_expr_uses_ident(e->u.if_expr.else_br, name);
    case STIGI_EXPR_WHILE:
      return ts_expr_uses_ident(e->u.while_expr.cond, name) ||
             ts_expr_uses_ident(e->u.while_expr.body, name);
    case STIGI_EXPR_CALL:
      if (ts_expr_uses_ident(e->u.call.callee, name)) return true;
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        if (ts_expr_uses_ident(e->u.call.args[i], name)) return true;
      }
      return false;
    case STIGI_EXPR_MATCH:
      if (ts_expr_uses_ident(e->u.match_expr.scrutinee, name)) return true;
      for (uint32_t i = 0; i < e->u.match_expr.scrutinee_count; i++) {
        if (ts_expr_uses_ident(e->u.match_expr.scrutinees[i], name)) return true;
      }
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (ts_expr_uses_ident(e->u.match_expr.arms[i].guard, name) ||
            ts_expr_uses_ident(e->u.match_expr.arms[i].body, name)) {
          return true;
        }
      }
      return false;
    case STIGI_EXPR_VAL:
      if (ts_expr_uses_ident(e->u.val_stmt.init, name)) return true;
      for (uint32_t i = 0; i < e->u.val_stmt.else_arm_count; i++) {
        if (ts_expr_uses_ident(e->u.val_stmt.else_arms[i].body, name)) return true;
      }
      return false;
    case STIGI_EXPR_VAR:
      return ts_expr_uses_ident(e->u.var_stmt.init, name);
    case STIGI_EXPR_ASSIGN:
      return (e->u.assign_stmt.name.len == name.len &&
              memcmp(e->u.assign_stmt.name.ptr, name.ptr, name.len) == 0) ||
             ts_expr_uses_ident(e->u.assign_stmt.value, name);
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      return ts_expr_uses_ident(e->u.index_expr.target, name) ||
             ts_expr_uses_ident(e->u.index_expr.index, name) ||
             ts_expr_uses_ident(e->u.index_expr.value, name);
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        if (ts_expr_uses_ident(e->u.handle_stmt.args[i], name)) return true;
      }
      return ts_expr_uses_ident(e->u.handle_stmt.body, name);
    case STIGI_EXPR_ASSERT:
      if (ts_expr_uses_ident(e->u.assert_expr.expr, name)) return true;
      for (uint32_t i = 0; i < e->u.assert_expr.facts.count; i++) {
        const StigiFactItem *it = &e->u.assert_expr.facts.items[i];
        if (it->kind == STIGI_FACT_ATOM) {
          for (uint32_t k = 0; k < it->arg_count; k++) {
            if (ts_expr_uses_ident(it->args[k], name)) return true;
          }
        } else {
          if (ts_expr_uses_ident(it->lhs, name) || ts_expr_uses_ident(it->rhs, name)) return true;
        }
      }
      return false;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (ts_expr_uses_ident(e->u.block.stmts[i], name)) return true;
      }
      return false;
    default:
      return false;
  }
}

static bool ts_expr_is_pure_simple(const StigiImage *img, uint32_t uidx, const StigiExpr *e) {
  if (e == NULL) return true;
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_LOCAL:
      return true;
    case STIGI_EXPR_UNOP:
      return ts_expr_is_pure_simple(img, uidx, e->u.unop.operand);
    case STIGI_EXPR_BINOP:
      return ts_expr_is_pure_simple(img, uidx, e->u.binop.lhs) &&
             ts_expr_is_pure_simple(img, uidx, e->u.binop.rhs);
    case STIGI_EXPR_CALL:
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        int32_t tgt = stigi_image_resolve_name(img, uidx, e->u.call.callee->u.ident.name.ptr,
                                               e->u.call.callee->u.ident.name.len);
        if (tgt >= 0 && ts_def_is_pure_computable(img, (uint32_t)tgt)) {
          for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
            if (!ts_expr_is_pure_simple(img, uidx, e->u.call.args[i])) return false;
          }
          return true;
        }
      }
      return false;
    default:
      return false;
  }
}

static StigiExpr *ts_fold_expr(const StigiImage *img, uint32_t uidx, const StigiExpr *e,
                               Arena *a, bool *changed) {
  if (e == NULL) return NULL;
  switch (e->kind) {
    case STIGI_EXPR_UNOP: {
      StigiExpr *op = ts_fold_expr(img, uidx, e->u.unop.operand, a, changed);
      if (e->u.unop.op == STIGI_OP_NOT && op != NULL && op->kind == STIGI_EXPR_LIT_BOOL) {
        *changed = true;
        return ts_make_bool_lit(a, !op->u.lit_bool.value, e->span);
      }
      if (op != e->u.unop.operand) {
        StigiExpr *ne = arena_alloc(a, sizeof *ne);
        if (ne == NULL) return (StigiExpr *)e;
        *ne = *e;
        ne->u.unop.operand = op;
        return ne;
      }
      return (StigiExpr *)e;
    }
    case STIGI_EXPR_BINOP: {
      StigiExpr *l = ts_fold_expr(img, uidx, e->u.binop.lhs, a, changed);
      StigiExpr *r = ts_fold_expr(img, uidx, e->u.binop.rhs, a, changed);
      if (l != NULL && r != NULL &&
          l->kind == STIGI_EXPR_LIT_INT && !l->u.lit_int.overflow &&
          r->kind == STIGI_EXPR_LIT_INT && !r->u.lit_int.overflow) {
        uint64_t a0 = l->u.lit_int.value;
        uint64_t b0 = r->u.lit_int.value;
        switch (e->u.binop.op) {
          case STIGI_OP_ADD:
            if (b0 <= UINT64_MAX - a0) { *changed = true; return ts_make_u64_lit(a, a0 + b0, e->span); }
            break;
          case STIGI_OP_SUB:
            if (a0 >= b0) { *changed = true; return ts_make_u64_lit(a, a0 - b0, e->span); }
            break;
          case STIGI_OP_MUL:
            if (a0 == 0 || b0 <= UINT64_MAX / a0) { *changed = true; return ts_make_u64_lit(a, a0 * b0, e->span); }
            break;
          case STIGI_OP_DIV:
            if (b0 != 0) { *changed = true; return ts_make_u64_lit(a, a0 / b0, e->span); }
            break;
          case STIGI_OP_MOD:
            if (b0 != 0) { *changed = true; return ts_make_u64_lit(a, a0 % b0, e->span); }
            break;
          case STIGI_OP_LT: *changed = true; return ts_make_bool_lit(a, a0 < b0, e->span);
          case STIGI_OP_LE: *changed = true; return ts_make_bool_lit(a, a0 <= b0, e->span);
          case STIGI_OP_GT: *changed = true; return ts_make_bool_lit(a, a0 > b0, e->span);
          case STIGI_OP_GE: *changed = true; return ts_make_bool_lit(a, a0 >= b0, e->span);
          case STIGI_OP_EQ: *changed = true; return ts_make_bool_lit(a, a0 == b0, e->span);
          case STIGI_OP_NE: *changed = true; return ts_make_bool_lit(a, a0 != b0, e->span);
          default: break;
        }
      }
      if (l != NULL && r != NULL &&
          l->kind == STIGI_EXPR_LIT_BOOL && r->kind == STIGI_EXPR_LIT_BOOL) {
        bool a0 = l->u.lit_bool.value, b0 = r->u.lit_bool.value;
        if (e->u.binop.op == STIGI_OP_AND) { *changed = true; return ts_make_bool_lit(a, a0 && b0, e->span); }
        if (e->u.binop.op == STIGI_OP_OR) { *changed = true; return ts_make_bool_lit(a, a0 || b0, e->span); }
        if (e->u.binop.op == STIGI_OP_EQ) { *changed = true; return ts_make_bool_lit(a, a0 == b0, e->span); }
        if (e->u.binop.op == STIGI_OP_NE) { *changed = true; return ts_make_bool_lit(a, a0 != b0, e->span); }
      }
      if (l != e->u.binop.lhs || r != e->u.binop.rhs) {
        StigiExpr *ne = arena_alloc(a, sizeof *ne);
        if (ne == NULL) return (StigiExpr *)e;
        *ne = *e;
        ne->u.binop.lhs = l;
        ne->u.binop.rhs = r;
        return ne;
      }
      return (StigiExpr *)e;
    }
    case STIGI_EXPR_IF: {
      StigiExpr *c = ts_fold_expr(img, uidx, e->u.if_expr.cond, a, changed);
      if (c != NULL && c->kind == STIGI_EXPR_LIT_BOOL) {
        *changed = true;
        return ts_fold_expr(img, uidx, c->u.lit_bool.value ? e->u.if_expr.then_br : e->u.if_expr.else_br,
                            a, changed);
      }
      StigiExpr *tb = ts_fold_expr(img, uidx, e->u.if_expr.then_br, a, changed);
      StigiExpr *eb = ts_fold_expr(img, uidx, e->u.if_expr.else_br, a, changed);
      if (c != e->u.if_expr.cond || tb != e->u.if_expr.then_br || eb != e->u.if_expr.else_br) {
        StigiExpr *ne = arena_alloc(a, sizeof *ne);
        if (ne == NULL) return (StigiExpr *)e;
        *ne = *e;
        ne->u.if_expr.cond = c;
        ne->u.if_expr.then_br = tb;
        ne->u.if_expr.else_br = eb;
        return ne;
      }
      return (StigiExpr *)e;
    }
    case STIGI_EXPR_CALL: {
      uint32_t argc = e->u.call.arg_count;
      StigiExpr **fargs = argc > 0 ? arena_alloc(a, (size_t)argc * sizeof *fargs) : NULL;
      bool any_arg_chg = false;
      bool all_lit = true;
      for (uint32_t i = 0; i < argc; i++) {
        StigiExpr *fa = ts_fold_expr(img, uidx, e->u.call.args[i], a, changed);
        if (fargs != NULL) fargs[i] = fa;
        if (fa != e->u.call.args[i]) any_arg_chg = true;
        if (fa == NULL || (fa->kind != STIGI_EXPR_LIT_INT && fa->kind != STIGI_EXPR_LIT_BOOL &&
                           fa->kind != STIGI_EXPR_LIT_CHAR)) {
          all_lit = false;
        }
      }
      if (all_lit && (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
                      e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
        int32_t tgt = stigi_image_resolve_name(img, uidx, e->u.call.callee->u.ident.name.ptr,
                                               e->u.call.callee->u.ident.name.len);
        if (tgt >= 0 && (uint32_t)tgt != uidx && ts_def_is_pure_computable(img, (uint32_t)tgt)) {
          StigiVal *vargs = argc > 0 ? malloc((size_t)argc * sizeof *vargs) : NULL;
          if (argc == 0 || vargs != NULL) {
            for (uint32_t i = 0; i < argc; i++) {
              memset(&vargs[i], 0, sizeof vargs[i]);
              const StigiExpr *fa = fargs[i];
              if (fa->kind == STIGI_EXPR_LIT_INT) {
                vargs[i].kind = STIGI_VAL_INT;
                vargs[i].is_unsigned = true;
                vargs[i].int_width = STIGI_INT_UNTYPED;
                vargs[i].u.u = fa->u.lit_int.value;
              } else if (fa->kind == STIGI_EXPR_LIT_BOOL) {
                vargs[i].kind = STIGI_VAL_BOOL;
                vargs[i].u.b = fa->u.lit_bool.value;
              } else if (fa->kind == STIGI_EXPR_LIT_CHAR) {
                vargs[i].kind = STIGI_VAL_INT;
                vargs[i].is_unsigned = true;
                vargs[i].int_width = STIGI_INT_U8;
                vargs[i].u.u = fa->u.lit_char.value;
              }
            }
            Arena eva;
            arena_init(&eva);
            StigiEvalResult er = stigi_eval_const_call(img, (uint32_t)tgt, vargs, argc, 4096ULL, &eva);
            free(vargs);
            if (er.status == STIGI_OK) {
              if (er.value.kind == STIGI_VAL_INT && er.value.is_unsigned) {
                uint64_t val = er.value.u.u;
                arena_free(&eva);
                *changed = true;
                return ts_make_u64_lit(a, val, e->span);
              }
              if (er.value.kind == STIGI_VAL_BOOL) {
                bool bv = er.value.u.b;
                arena_free(&eva);
                *changed = true;
                return ts_make_bool_lit(a, bv, e->span);
              }
            }
            arena_free(&eva);
          }
        }
      }
      if (any_arg_chg) {
        StigiExpr *ne = arena_alloc(a, sizeof *ne);
        if (ne == NULL) return (StigiExpr *)e;
        *ne = *e;
        ne->u.call.args = fargs;
        return ne;
      }
      return (StigiExpr *)e;
    }
    case STIGI_EXPR_VAL: {
      StigiExpr *ni = ts_fold_expr(img, uidx, e->u.val_stmt.init, a, changed);
      if (ni != e->u.val_stmt.init) {
        StigiExpr *ne = arena_alloc(a, sizeof *ne);
        if (ne == NULL) return (StigiExpr *)e;
        *ne = *e;
        ne->u.val_stmt.init = ni;
        return ne;
      }
      return (StigiExpr *)e;
    }
    case STIGI_EXPR_BLOCK: {
      uint32_t n = e->u.block.count;
      StigiExpr **stmts = n > 0 ? arena_alloc(a, (size_t)n * sizeof *stmts) : NULL;
      if (n > 0 && stmts == NULL) return (StigiExpr *)e;
      bool blk_chg = false;
      for (uint32_t i = 0; i < n; i++) {
        stmts[i] = ts_fold_expr(img, uidx, e->u.block.stmts[i], a, changed);
        if (stmts[i] != e->u.block.stmts[i]) blk_chg = true;
      }
      /* Eliminate unused pure local `val x = expr` statements */
      uint32_t w = 0;
      for (uint32_t i = 0; i < n; i++) {
        const StigiExpr *s = stmts[i];
        if (i + 1U < n && s != NULL && s->kind == STIGI_EXPR_VAL &&
            s->u.val_stmt.else_arm_count == 0 && s->u.val_stmt.pat.kind == STIGI_PAT_VAR &&
            ts_expr_is_pure_simple(img, uidx, s->u.val_stmt.init)) {
          bool used = false;
          for (uint32_t j = i + 1U; j < n; j++) {
            if (ts_expr_uses_ident(stmts[j], s->u.val_stmt.pat.name)) {
              used = true;
              break;
            }
          }
          if (!used) {
            blk_chg = true;
            *changed = true;
            continue;
          }
        }
        stmts[w++] = stmts[i];
      }
      if (!blk_chg) return (StigiExpr *)e;
      if (w == 1U) return stmts[0];
      StigiExpr *nb = arena_alloc(a, sizeof *nb);
      if (nb == NULL) return (StigiExpr *)e;
      *nb = *e;
      nb->u.block.stmts = stmts;
      nb->u.block.count = w;
      return nb;
    }
    default:
      return (StigiExpr *)e;
  }
}

Status stigi_door_gc(StigiDoor *door, uint32_t *out_removed_units,
                     StigiDoorResult *out, Diag *diag) {
  if (door == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_gc: invalid arguments");
  }
  if (out_removed_units != NULL) *out_removed_units = 0U;
  if (door->src_count == 0) {
    out->accepted = true;
    return STIGI_OK;
  }

  /* Phase 1 (S127): Comptime partial evaluation & constant folding on non-Test units */
  uint32_t n = door->image.unit_count < door->src_count ? door->image.unit_count : door->src_count;
  char **folded_srcs = calloc(n, sizeof *folded_srcs);
  size_t *folded_lens = calloc(n, sizeof *folded_lens);
  bool any_folded = false;
  if (folded_srcs != NULL && folded_lens != NULL) {
    Arena fa;
    arena_init(&fa);
    for (uint32_t i = 0; i < n; i++) {
      const StigiUnit *u = door->image.units[i].ast;
      const StigiDecl *d = u->decl;
      bool chg = false;
      StigiUnit mod_u = *u;
      StigiDecl mod_d = *d;
      if (d->kind == STIGI_DECL_DEF && d->u.def_decl.clause_count == 0 &&
          d->u.def_decl.body != NULL) {
        StigiExpr *fb = ts_fold_expr(&door->image, i, d->u.def_decl.body, &fa, &chg);
        if (chg && fb != NULL) {
          mod_d.u.def_decl.body = fb;
          mod_u.decl = &mod_d;
        }
      } else if (d->kind == STIGI_DECL_VAL && d->u.val_decl.body != NULL) {
        StigiExpr *fb = ts_fold_expr(&door->image, i, d->u.val_decl.body, &fa, &chg);
        if (chg && fb != NULL) {
          mod_d.u.val_decl.body = fb;
          mod_u.decl = &mod_d;
        }
      }
      if (chg) {
        char *printed = NULL;
        size_t plen = 0;
        Diag pd;
        diag_clear(&pd);
        if (stigi_print_unit(&mod_u, &printed, &plen, &fa, &pd) == STIGI_OK && printed != NULL) {
          char *dup = malloc(plen + 1U);
          if (dup != NULL) {
            memcpy(dup, printed, plen);
            dup[plen] = '\0';
            folded_srcs[i] = dup;
            folded_lens[i] = plen;
            any_folded = true;
          }
        }
      }
    }
    arena_free(&fa);
  }

  /* Phase 2 (S126/S127): Recompute reachability after folding and prune unreachable units */
  if (any_folded) {
    StigiImage tmp_img;
    StigiNameDiags tmp_nd;
    stigi_image_init(&tmp_img);
    stigi_name_diags_init(&tmp_nd);
    Diag td;
    diag_clear(&td);
    bool tmp_ok = true;
    for (uint32_t i = 0; i < n && tmp_ok; i++) {
      const char *s = folded_srcs[i] != NULL ? folded_srcs[i] : door->unit_srcs[i];
      size_t sl = folded_srcs[i] != NULL ? folded_lens[i] : door->unit_lens[i];
      if (stigi_image_add_source(&tmp_img, s, sl, &tmp_nd, &td) != STIGI_OK) tmp_ok = false;
    }
    if (tmp_ok && stigi_image_resolve(&tmp_img, &tmp_nd, &td) == STIGI_OK) {
      const char **kept_srcs = malloc((size_t)n * sizeof *kept_srcs);
      size_t *kept_lens = malloc((size_t)n * sizeof *kept_lens);
      if (kept_srcs != NULL && kept_lens != NULL) {
        uint32_t w = 0;
        for (uint32_t i = 0; i < n && i < tmp_img.unit_count; i++) {
          if (tmp_img.units[i].reachable) {
            kept_srcs[w] = folded_srcs[i] != NULL ? folded_srcs[i] : door->unit_srcs[i];
            kept_lens[w] = folded_srcs[i] != NULL ? folded_lens[i] : door->unit_lens[i];
            w++;
          }
        }
        /* Soundness gate: commit folded+pruned image ONLY if full door accepts it! */
        StigiDoorResult try_r;
        stigi_door_result_init(&try_r);
        Diag try_d;
        diag_clear(&try_d);
        if (run_transaction(door, kept_srcs, kept_lens, w, NULL, 0, true, true,
                            &try_r, &try_d) == STIGI_OK &&
            try_r.accepted) {
          if (out_removed_units != NULL) *out_removed_units = n > w ? (n - w) : 0U;
          out->accepted = true;
          stigi_door_result_free(&try_r);
          free(kept_srcs);
          free(kept_lens);
          stigi_name_diags_free(&tmp_nd);
          stigi_image_free(&tmp_img);
          for (uint32_t i = 0; i < n; i++) free(folded_srcs[i]);
          free(folded_srcs);
          free(folded_lens);
          return STIGI_OK;
        }
        stigi_door_result_free(&try_r);
      }
      free(kept_srcs);
      free(kept_lens);
    }
    stigi_name_diags_free(&tmp_nd);
    stigi_image_free(&tmp_img);
  }
  if (folded_srcs != NULL) {
    for (uint32_t i = 0; i < n; i++) free(folded_srcs[i]);
  }
  free(folded_srcs);
  free(folded_lens);

  /* Fallback / non-folded path: exact reachability GC from roots */
  uint32_t keep_cnt = 0;
  for (uint32_t i = 0; i < door->image.unit_count && i < door->src_count; i++) {
    if (door->image.units[i].reachable) keep_cnt++;
  }
  uint32_t removed = door->src_count > keep_cnt ? (door->src_count - keep_cnt) : 0U;
  if (out_removed_units != NULL) *out_removed_units = removed;
  if (removed == 0U) {
    out->accepted = true;
    return STIGI_OK;
  }

  const char **kept_srcs = keep_cnt > 0 ? malloc((size_t)keep_cnt * sizeof *kept_srcs) : NULL;
  size_t *kept_lens = keep_cnt > 0 ? malloc((size_t)keep_cnt * sizeof *kept_lens) : NULL;
  if (keep_cnt > 0 && (kept_srcs == NULL || kept_lens == NULL)) {
    free(kept_srcs);
    free(kept_lens);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_door_gc OOM");
  }
  uint32_t w = 0;
  for (uint32_t i = 0; i < door->image.unit_count && i < door->src_count; i++) {
    if (door->image.units[i].reachable) {
      kept_srcs[w] = door->unit_srcs[i];
      kept_lens[w] = door->unit_lens[i];
      w++;
    }
  }
  Status st = run_transaction(door, kept_srcs, kept_lens, w, NULL, 0, true, true, out, diag);
  free(kept_srcs);
  free(kept_lens);
  return st;
}

Status stigi_door_export(StigiDoor *door, const char *dir, StigiExportStats *stats, Diag *diag) {
  if (door == NULL || dir == NULL || stats == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_export: invalid arguments");
  }
  if (mkdir(dir, 0755) != 0) {
    return diag_set(diag, STIGI_ERR_IO, errno, "cannot create %s (the export directory must be new)", dir);
  }
  char rules_path[1024];
  int rn = snprintf(rules_path, sizeof rules_path, "%s/rules.dl", dir);
  if (rn < 0 || (size_t)rn >= sizeof rules_path) {
    return diag_set(diag, STIGI_ERR_IO, -1, "export path too long");
  }
  FILE *f = fopen(rules_path, "w");
  if (f == NULL) return diag_set(diag, STIGI_ERR_IO, errno, "cannot write %s", rules_path);
  if (fwrite(door->rules_dl, 1, door->rules_dl_len, f) != door->rules_dl_len) {
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_IO, errno, "write error on %s", rules_path);
  }
  if (fclose(f) != 0) return diag_set(diag, STIGI_ERR_IO, errno, "close failed on %s", rules_path);

  ExportCtx ex = {dir, 0, 0, 0, 0, 0};
  Status st = STIGI_OK;
  const StigiImage *img = &door->image;
  for (uint32_t i = 0; i < img->unit_count && st == STIGI_OK; i++) {
    if (img->units[i].ast->decl->kind == STIGI_DECL_NEG_TEST) continue;
    StigiFactSet fs;
    stigi_facts_init(&fs);
    st = unit_fact_set(img, i, door->kernel_mode, &fs, diag);
    bool wrote = false;
    if (st == STIGI_OK) st = write_fact_dir(&ex, dir, img->units[i].unit_id, &fs, door->rules_dl, &wrote, diag);
    if (st == STIGI_OK && wrote) ex.unit_dirs++;
    stigi_facts_free(&fs);
  }
  for (uint32_t i = 0; i < img->unit_count && st == STIGI_OK; i++) {
    if (img->units[i].ast->decl->kind != STIGI_DECL_NEG_TEST) continue;
    StigiDoorResult ignored;
    stigi_door_result_init(&ignored);
    st = check_negtest_unit(door, &door->dl, door->rules_dl, img, i, &ignored, &ex, diag);
    stigi_door_result_free(&ignored);
  }
  stats->unit_dirs = ex.unit_dirs;
  stats->negtest_dirs = ex.negtest_dirs;
  stats->negtest_local_dirs = ex.negtest_local_dirs;
  stats->negtests_without_datalog = ex.negtests_without_datalog;
  stats->rows = ex.rows;
  return st;
}

Status stigi_door_unit_key(const StigiDoor *door, uint32_t uidx, uint8_t out[32], Diag *diag) {
  if (door == NULL || uidx >= door->image.unit_count) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_unit_key: invalid arguments");
  }
  uint8_t rules_hash[32];
  Status st = compute_active_rules_hash(door, &door->image, rules_hash, diag);
  if (st != STIGI_OK) return st;
  effective_verdict_key(door->image.units[uidx].verdict_key, rules_hash, door->seed_id, door->row_budget,
                        door->kernel_mode, out);
  return STIGI_OK;
}
