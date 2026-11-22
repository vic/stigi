#define _POSIX_C_SOURCE 200809L
#include "blake3.h"
#include "common.h"
#include "datalog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_fails = 0;

static void check(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fails++;
  }
}

static void to_hex(const uint8_t hash[32], char out[65]) {
  static const char HEX[] = "0123456789abcdef";
  for (size_t i = 0; i < 32; i++) {
    out[i * 2] = HEX[(hash[i] >> 4) & 0x0f];
    out[i * 2 + 1] = HEX[hash[i] & 0x0f];
  }
  out[64] = '\0';
}

static long read_rss_kb(void) {
  FILE *f = fopen("/proc/self/status", "r");
  if (f == NULL) return -1;
  char line[256];
  long kb = -1;
  while (fgets(line, sizeof line, f) != NULL) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      kb = strtol(line + 6, NULL, 10);
      break;
    }
  }
  (void)fclose(f);
  return kb;
}

static void test_blake3(void) {
  uint8_t h[32];
  char hex[65];

  blake3_hash((const uint8_t *)"", 0, h);
  to_hex(h, hex);
  check(strcmp(hex, "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262") == 0,
        "blake3(\"\") matches b3sum");

  blake3_hash((const uint8_t *)"stigi", 5, h);
  to_hex(h, hex);
  check(strcmp(hex, "eacc179ac109dcb56a36fb4606b0f4d66aa66334ff242f3faff704b4b44d4597") == 0,
        "blake3(\"stigi\") matches b3sum");

  uint8_t buf[3000];
  memset(buf, 'a', sizeof buf);

  blake3_hash(buf, 64, h);
  to_hex(h, hex);
  check(strcmp(hex, "472c51290d607f100d2036fdcedd7590bba245e9adeb21364a063b7bb4ca81c7") == 0,
        "blake3(64 * 'a') matches b3sum");

  blake3_hash(buf, 1024, h);
  to_hex(h, hex);
  check(strcmp(hex, "5a1c9e5d85d9898297037e8e24f69bb0e604a84c91c3b3ef4784a374812900d9") == 0,
        "blake3(1024 * 'a') matches b3sum");

  blake3_hash(buf, 1025, h);
  to_hex(h, hex);
  check(strcmp(hex, "c59d2e12583df14d951e757a42f1734d355c8c5b1db6b6a33ab2bfabeed40c7d") == 0,
        "blake3(1025 * 'a') matches b3sum");

  blake3_hash(buf, 3000, h);
  to_hex(h, hex);
  check(strcmp(hex, "a012abdd339b966bfbf116187ba42db7a6aea0ca9d47219b0f88efdf99cd1b2e") == 0,
        "blake3(3000 * 'a') matches b3sum");
}

static void test_arena_and_intern(void) {
  Arena a;
  arena_init(&a);

  void *p1 = arena_alloc(&a, 17);
  void *p2 = arena_alloc(&a, 33);
  check(p1 != NULL && p2 != NULL && p1 != p2, "arena_alloc basic");
  check(((uintptr_t)p1 % _Alignof(max_align_t)) == 0, "arena_alloc p1 aligned");
  check(((uintptr_t)p2 % _Alignof(max_align_t)) == 0, "arena_alloc p2 aligned");

  for (int i = 0; i < 40; i++) {
    void *blk = arena_alloc(&a, 4096);
    check(blk != NULL, "arena_alloc multi-block");
    memset(blk, 0x5a, 4096);
  }
  arena_reset(&a);
  void *p3 = arena_alloc(&a, 64);
  check(p3 != NULL, "arena_alloc after reset");
  void *big = arena_alloc(&a, 128U * 1024U);
  check(big != NULL, "arena_alloc oversized block");
  memset(big, 0x33, 128U * 1024U);
  arena_free(&a);

  InternPool pool;
  intern_init(&pool);
  Diag diag;
  diag_clear(&diag);

  uint32_t id_a1 = 999, id_a2 = 999, id_b = 999, id_empty = 999;
  check(intern_str(&pool, "alpha", 5, &id_a1, &diag) == STIGI_OK, "intern alpha");
  check(intern_str(&pool, "beta", 4, &id_b, &diag) == STIGI_OK, "intern beta");
  check(intern_str(&pool, "alpha", 5, &id_a2, &diag) == STIGI_OK, "intern alpha again");
  check(intern_str(&pool, "", 0, &id_empty, &diag) == STIGI_OK, "intern empty");
  check(id_a1 == 0 && id_b == 1 && id_a2 == id_a1 && id_empty == 2, "intern deduplication & dense IDs");

  size_t len = 0;
  const char *got = intern_get(&pool, id_a1, &len);
  check(got != NULL && len == 5 && strcmp(got, "alpha") == 0, "intern_get alpha");
  check(intern_get(&pool, 9999U, &len) == NULL, "intern_get out of bounds returns NULL");
  check(intern_str(&pool, NULL, 3, &id_a1, &diag) == STIGI_ERR_INVALID, "intern_str NULL with len>0 returns error");

  for (int i = 0; i < 300; i++) {
    char sbuf[32];
    int n = snprintf(sbuf, sizeof sbuf, "sym_%d", i);
    uint32_t id = 0, id_dup = 0;
    check(intern_str(&pool, sbuf, (size_t)n, &id, &diag) == STIGI_OK, "intern bulk");
    check(intern_str(&pool, sbuf, (size_t)n, &id_dup, &diag) == STIGI_OK && id == id_dup, "intern bulk dup");
  }

  intern_reset(&pool);
  uint32_t id_after_reset = 999;
  check(intern_str(&pool, "alpha", 5, &id_after_reset, &diag) == STIGI_OK && id_after_reset == 0,
        "intern_reset resets IDs to 0");
  intern_free(&pool);
}

static void test_dl_u64_bounds(void) {
  static const char *PROG =
      ".decl check_add(name: symbol, delta_from_max: number, k: number)\n"
      ".input check_add\n"
      ".decl u64_limit(m: number)\n"
      "u64_limit(18446744073709551615).\n"
      ".decl ok(name: symbol)\n"
      ".output ok\n"
      ".decl overflow(name: symbol)\n"
      ".output overflow\n"
      "ok(name) :- check_add(name, d, k), u64_limit(m), d >= 0, k >= 0, (m - d) + k <= m.\n"
      "overflow(name) :- check_add(name, d, k), u64_limit(m), (d < 0 ; k < 0 ; (m - d) + k > m).\n";

  StigiDl dl;
  Diag diag;
  check(stigi_dl_open(&dl, PROG, strlen(PROG), "u64_bounds.dl", 10000, 1, &diag) == STIGI_OK,
        "stigi_dl_open u64_bounds");
  if (dl.sess == NULL) return;

  int rel_in = stigi_dl_rel_id(&dl, "check_add");
  int rel_ok = stigi_dl_rel_id(&dl, "ok");
  int rel_ov = stigi_dl_rel_id(&dl, "overflow");
  check(rel_in >= 0 && rel_ok >= 0 && rel_ov >= 0, "u64_bounds rel ids");

  uint32_t s_exact = 0, s_over = 0, s_max1 = 0;
  check(stigi_dl_intern(&dl, "exact_max", 9, &s_exact, &diag) == STIGI_OK, "intern exact_max");
  check(stigi_dl_intern(&dl, "over_max", 8, &s_over, &diag) == STIGI_OK, "intern over_max");
  check(stigi_dl_intern(&dl, "at_max_plus_1", 13, &s_max1, &diag) == STIGI_OK, "intern at_max_plus_1");

  int64_t r1[3] = { (int64_t)s_exact, 1, 1 };
  int64_t r2[3] = { (int64_t)s_over, 1, 2 };
  int64_t r3[3] = { (int64_t)s_max1, 0, 1 };
  check(stigi_dl_add(&dl, rel_in, r1, 3, &diag) == STIGI_OK, "add exact_max");
  check(stigi_dl_add(&dl, rel_in, r2, 3, &diag) == STIGI_OK, "add over_max");
  check(stigi_dl_add(&dl, rel_in, r3, 3, &diag) == STIGI_OK, "add at_max_plus_1");

  check(stigi_dl_eval(&dl, &diag) == STIGI_OK, "eval u64_bounds");
  check(stigi_dl_count(&dl, rel_ok) == 1, "u64_bounds ok count == 1");
  check(stigi_dl_count(&dl, rel_ov) == 2, "u64_bounds overflow count == 2");

  int64_t out_row[1] = { -1 };
  check(stigi_dl_read(&dl, rel_ok, 0, out_row, 1, &diag) == STIGI_OK && out_row[0] == (int64_t)s_exact,
        "u64_bounds ok row == exact_max");
  stigi_dl_close(&dl);
}

static const char *TC_PROG =
    ".decl edge(x: symbol, y: symbol)\n"
    ".input edge\n"
    ".decl tc(x: symbol, y: symbol)\n"
    ".output tc\n"
    "tc(x, y) :- edge(x, y).\n"
    "tc(x, z) :- tc(x, y), edge(y, z).\n";

static void test_dl_1000_frames(void) {
  StigiDl dl;
  Diag diag;
  check(stigi_dl_open(&dl, TC_PROG, strlen(TC_PROG), "tc.dl", 1000000, 1, &diag) == STIGI_OK,
        "stigi_dl_open tc");
  if (dl.sess == NULL) return;

  int edge_id = stigi_dl_rel_id(&dl, "edge");
  int tc_id = stigi_dl_rel_id(&dl, "tc");
  check(edge_id >= 0 && tc_id >= 0, "tc rel ids");
  check(stigi_dl_rel_ncols(&dl, edge_id) == 2 && stigi_dl_rel_ncols(&dl, tc_id) == 2, "tc ncols == 2");

  uint32_t sym[10];
  for (int i = 0; i < 10; i++) {
    char buf[16];
    int n = snprintf(buf, sizeof buf, "n%d", i);
    check(stigi_dl_intern(&dl, buf, (size_t)n, &sym[i], &diag) == STIGI_OK, "stigi_dl_intern");
    size_t slen = 0;
    const char *sp = stigi_dl_sym_str(&dl, sym[i], &slen);
    check(sp != NULL && slen == (size_t)n && memcmp(sp, buf, slen) == 0, "stigi_dl_sym_str");
  }

  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  long rss_f10 = -1, rss_f1000 = -1;

  for (int frame = 1; frame <= 1000; frame++) {
    check(stigi_dl_reset(&dl, &diag) == STIGI_OK, "stigi_dl_reset");
    for (int i = 0; i < 9; i++) {
      int64_t row[2] = { (int64_t)sym[i], (int64_t)sym[i + 1] };
      check(stigi_dl_add(&dl, edge_id, row, 2, &diag) == STIGI_OK, "stigi_dl_add");
    }
    check(stigi_dl_eval(&dl, &diag) == STIGI_OK, "stigi_dl_eval");
    check(stigi_dl_count(&dl, tc_id) == 45, "stigi_dl_count == 45");
    if (frame == 10) rss_f10 = read_rss_kb();
    if (frame == 1000) rss_f1000 = read_rss_kb();
  }

  clock_gettime(CLOCK_MONOTONIC, &t1);
  double ms = (double)(t1.tv_sec - t0.tv_sec) * 1000.0 + (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;
  long delta = (rss_f10 >= 0 && rss_f1000 >= 0) ? (rss_f1000 - rss_f10) : 0L;
  printf("c1_test 1000_frames wall=%.2fms rss_f10=%ldKB rss_f1000=%ldKB rss_delta=%ldKB\n",
         ms, rss_f10, rss_f1000, delta);
  stigi_dl_close(&dl);
}

static void test_dl_budget_refusal(void) {
  StigiDl dl;
  Diag diag;
  check(stigi_dl_open(&dl, TC_PROG, strlen(TC_PROG), "tc_budget.dl", 10, 1, &diag) == STIGI_OK,
        "stigi_dl_open budget");
  if (dl.sess == NULL) return;

  int edge_id = stigi_dl_rel_id(&dl, "edge");
  uint32_t sym[10];
  for (int i = 0; i < 10; i++) {
    char buf[16];
    int n = snprintf(buf, sizeof buf, "b%d", i);
    check(stigi_dl_intern(&dl, buf, (size_t)n, &sym[i], &diag) == STIGI_OK, "intern budget");
  }
  for (int i = 0; i < 9; i++) {
    int64_t row[2] = { (int64_t)sym[i], (int64_t)sym[i + 1] };
    check(stigi_dl_add(&dl, edge_id, row, 2, &diag) == STIGI_OK, "add budget");
  }
  Status st = stigi_dl_eval(&dl, &diag);
  check(st == STIGI_ERR_DATALOG && diag.dl_code == TUPU_CODE_DATALOG_BUDGET,
        "budget exceeded returns TUPU_CODE_DATALOG_BUDGET");
  stigi_dl_close(&dl);
}

int main(void) {
  test_blake3();
  test_arena_and_intern();
  test_dl_u64_bounds();
  test_dl_1000_frames();
  test_dl_budget_refusal();
  printf("c1_test failures=%d\n", g_fails);
  return g_fails == 0 ? 0 : 1;
}
