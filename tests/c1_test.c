#define _POSIX_C_SOURCE 200809L
#include "blake3.h"
#include "common.h"
#include "datalog.h"

#include <stdbool.h>
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

  /* Multi-chunk Merkle tree >= 8192B vectors verified against b3sum */
  uint8_t *big = malloc(65536);
  check(big != NULL, "blake3 big malloc");
  if (big != NULL) {
    memset(big, 'a', 65536);
    blake3_hash(big, 8192, h);
    to_hex(h, hex);
    check(strcmp(hex, "4b6d9ef12ab5a1ab11e7441f0460fb4fc7e03e5dfe75a37dec5c43fabbb03b6e") == 0,
          "blake3(8192 * 'a') matches b3sum");

    blake3_hash(big, 10240, h);
    to_hex(h, hex);
    check(strcmp(hex, "9afd0ba102b2cc68be10ba4d383b3139b97ed36d425b82631a7a1e2424088f7e") == 0,
          "blake3(10240 * 'a') matches b3sum");

    blake3_hash(big, 65536, h);
    to_hex(h, hex);
    check(strcmp(hex, "028712f89696339b8c9ecbb821977d434235d8ae98821d29094933439b2b402b") == 0,
          "blake3(65536 * 'a') matches b3sum");
    free(big);
  }

  /* NULL with len > 0 must not silently hash zeros as a valid message */
  uint8_t h_null[32], h_zero[32];
  uint8_t zeros[16];
  memset(zeros, 0, sizeof zeros);
  blake3_hash(NULL, 16, h_null);
  blake3_hash(zeros, 16, h_zero);
  check(memcmp(h_null, h_zero, 32) != 0, "blake3(NULL, 16) does not hash fake zeros");
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
  check(((uintptr_t)p3 % 64U) == 0, "arena_alloc(64) is 64-byte cache-line aligned (memory.md 8)");
  ArenaBlock *cur_before_big = a.cur;
  void *big = arena_alloc(&a, 128U * 1024U);
  check(big != NULL && ((uintptr_t)big % 64U) == 0, "arena_alloc oversized block 64-byte aligned");
  check(a.cur == cur_before_big, "oversized block does not abandon free space in a.cur");
  memset(big, 0x33, 128U * 1024U);
  void *s1 = arena_alloc_aligned(&a, 5, 1);
  void *s2 = arena_alloc_aligned(&a, 5, 1);
  check(s1 != NULL && s2 != NULL && (uintptr_t)s2 - (uintptr_t)s1 == 5U,
        "arena_alloc_aligned(..., 1) packs without 16-byte waste");
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
      ".decl check_add(name: symbol, base: number, k: number, limit: number)\n"
      ".input check_add\n"
      ".decl ok(name: symbol, sum: number)\n"
      ".output ok\n"
      ".decl overflow(name: symbol, sum: number)\n"
      ".output overflow\n"
      "ok(name, s) :- check_add(name, b, k, lim), s = b + k, s <= lim.\n"
      "overflow(name, s) :- check_add(name, b, k, lim), s = b + k, s > lim.\n";

  StigiDl dl;
  Diag diag;
  check(stigi_dl_open_ex(&dl, PROG, strlen(PROG), "u64_bounds.dl", 10000, 1, 1, &diag) == STIGI_OK,
        "stigi_dl_open_ex u64_bounds with provenance");
  if (dl.sess == NULL) return;

  int rel_in = stigi_dl_rel_id(&dl, "check_add");
  int rel_ok = stigi_dl_rel_id(&dl, "ok");
  int rel_ov = stigi_dl_rel_id(&dl, "overflow");
  check(rel_in >= 0 && rel_ok >= 0 && rel_ov >= 0, "u64_bounds rel ids");

  uint32_t s_exact = 0, s_over = 0, s_max1 = 0, s_neg = 0;
  check(stigi_dl_intern(&dl, "exact_max", 9, &s_exact, &diag) == STIGI_OK, "intern exact_max");
  check(stigi_dl_intern(&dl, "over_max", 8, &s_over, &diag) == STIGI_OK, "intern over_max");
  check(stigi_dl_intern(&dl, "at_max_plus_1", 13, &s_max1, &diag) == STIGI_OK, "intern at_max_plus_1");
  check(stigi_dl_intern(&dl, "i64_min_ok", 10, &s_neg, &diag) == STIGI_OK, "intern i64_min_ok");

  TupuI128 lim_u64 = tupu_i128_from_u64(UINT64_MAX);
  TupuI128 r1[4] = { tupu_i128_from_u64(s_exact), tupu_i128_from_u64(UINT64_MAX - 1ULL), tupu_i128_from_u64(1), lim_u64 };
  TupuI128 r2[4] = { tupu_i128_from_u64(s_over), tupu_i128_from_u64(UINT64_MAX - 1ULL), tupu_i128_from_u64(2), lim_u64 };
  TupuI128 r3[4] = { tupu_i128_from_u64(s_max1), tupu_i128_from_u64(UINT64_MAX), tupu_i128_from_u64(1), lim_u64 };
  TupuI128 r4[4] = { tupu_i128_from_u64(s_neg), tupu_i128_from_i64(INT64_MIN), tupu_i128_from_i64(0), tupu_i128_from_i64(0) };

  check(stigi_dl_add_i128(&dl, rel_in, r1, 4, &diag) == STIGI_OK, "add_i128 exact_max");
  check(stigi_dl_add_i128(&dl, rel_in, r2, 4, &diag) == STIGI_OK, "add_i128 over_max");
  check(stigi_dl_add_i128(&dl, rel_in, r3, 4, &diag) == STIGI_OK, "add_i128 at_max_plus_1");
  check(stigi_dl_add_i128(&dl, rel_in, r4, 4, &diag) == STIGI_OK, "add_i128 i64_min_ok");

  check(stigi_dl_eval(&dl, &diag) == STIGI_OK, "eval u64_bounds i128");
  check(stigi_dl_replay(&dl, &diag) == STIGI_OK, "O6 provenance replay passes");
  check(stigi_dl_count(&dl, rel_ok) == 2, "u64_bounds ok count == 2");
  check(stigi_dl_count(&dl, rel_ov) == 2, "u64_bounds overflow count == 2");

  bool saw_u64_max = false, saw_i64_min = false, saw_2p64 = false;
  for (uint32_t i = 0; i < stigi_dl_count(&dl, rel_ok); i++) {
    TupuI128 row[2];
    check(stigi_dl_read_i128(&dl, rel_ok, i, row, 2, &diag) == STIGI_OK, "read_i128 ok");
    if (row[0].lo == s_exact && row[1].hi == 0 && row[1].lo == UINT64_MAX) saw_u64_max = true;
    if (row[0].lo == s_neg && row[1].hi == -1 && row[1].lo == (uint64_t)INT64_MIN) saw_i64_min = true;
  }
  for (uint32_t i = 0; i < stigi_dl_count(&dl, rel_ov); i++) {
    TupuI128 row[2];
    check(stigi_dl_read_i128(&dl, rel_ov, i, row, 2, &diag) == STIGI_OK, "read_i128 overflow");
    if (row[0].lo == s_max1 && row[1].hi == 1 && row[1].lo == 0ULL) saw_2p64 = true;
  }
  check(saw_u64_max, "exact U64_MAX round-tripped via TupuI128");
  check(saw_i64_min, "exact I64_MIN round-tripped via TupuI128");
  check(saw_2p64, "exact 2^64 (hi=1, lo=0) computed without 64-bit wrap");
  stigi_dl_close(&dl);
}

static void test_dl_refusals_and_parallel(void) {
  Diag diag;
  StigiDl dl;

  /* 1. rule-unsafe */
  const char *unsafe_prog =
      ".decl a(x: number)\n.input a\n.decl b(x: number, y: number)\n.output b\n"
      "b(x, y) :- a(x).\n";
  Status st = stigi_dl_open(&dl, unsafe_prog, strlen(unsafe_prog), "unsafe.dl", 1000, 1, &diag);
  check(st == STIGI_ERR_DATALOG && diag.dl_code == TUPU_CODE_RULE_UNSAFE,
        "rule-unsafe returns TUPU_CODE_RULE_UNSAFE");

  /* 2. negation-cycle */
  const char *neg_prog =
      ".decl base(x: number)\n.input base\n"
      ".decl p(x: number)\n.decl q(x: number)\n.output p\n"
      "p(x) :- base(x), !q(x).\n"
      "q(x) :- base(x), !p(x).\n";
  st = stigi_dl_open(&dl, neg_prog, strlen(neg_prog), "neg.dl", 1000, 1, &diag);
  check(st == STIGI_ERR_DATALOG && diag.dl_code == TUPU_CODE_NEGATION_CYCLE,
        "negation-cycle returns TUPU_CODE_NEGATION_CYCLE");

  /* 3. datalog-arith (128-bit overflow) */
  const char *arith_prog =
      ".decl seed(x: number)\n.input seed\n"
      ".decl big(x: number)\n.output big\n"
      "big(y) :- seed(x), y = x + x.\n";
  check(stigi_dl_open(&dl, arith_prog, strlen(arith_prog), "arith.dl", 1000, 1, &diag) == STIGI_OK,
        "open arith_ov");
  if (dl.sess != NULL) {
    int rel_s = stigi_dl_rel_id(&dl, "seed");
    TupuI128 max128 = { UINT64_MAX, INT64_MAX };
    check(stigi_dl_add_i128(&dl, rel_s, &max128, 1, &diag) == STIGI_OK, "add I128_MAX");
    st = stigi_dl_eval(&dl, &diag);
    check(st == STIGI_ERR_DATALOG && diag.dl_code == TUPU_CODE_DATALOG_ARITH,
          "128-bit overflow returns TUPU_CODE_DATALOG_ARITH");
    stigi_dl_close(&dl);
  }

  /* 4. Multi-threaded determinism (S80): nthreads=4 matches nthreads=1 */
  const char *tc_prog =
      ".decl edge(x: number, y: number)\n.input edge\n"
      ".decl tc(x: number, y: number)\n.output tc\n"
      "tc(x, y) :- edge(x, y).\n"
      "tc(x, z) :- tc(x, y), edge(y, z).\n";
  StigiDl dl1, dl4;
  check(stigi_dl_open(&dl1, tc_prog, strlen(tc_prog), "tc1.dl", 1000000, 1, &diag) == STIGI_OK, "open tc1");
  check(stigi_dl_open(&dl4, tc_prog, strlen(tc_prog), "tc4.dl", 1000000, 4, &diag) == STIGI_OK, "open tc4");
  if (dl1.sess != NULL && dl4.sess != NULL) {
    int e1 = stigi_dl_rel_id(&dl1, "edge"), t1 = stigi_dl_rel_id(&dl1, "tc");
    int e4 = stigi_dl_rel_id(&dl4, "edge"), t4 = stigi_dl_rel_id(&dl4, "tc");
    for (int i = 0; i < 60; i++) {
      int64_t row[2] = { i, i + 1 };
      (void)stigi_dl_add(&dl1, e1, row, 2, &diag);
      (void)stigi_dl_add(&dl4, e4, row, 2, &diag);
    }
    check(stigi_dl_eval(&dl1, &diag) == STIGI_OK, "eval tc1");
    check(stigi_dl_eval(&dl4, &diag) == STIGI_OK, "eval tc4");
    uint32_t c1 = stigi_dl_count(&dl1, t1);
    uint32_t c4 = stigi_dl_count(&dl4, t4);
    check(c1 == 1830U && c1 == c4, "S80 nthreads=4 count matches nthreads=1 (1830)");
  }
  stigi_dl_close(&dl1);
  stigi_dl_close(&dl4);
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
#if !defined(__SANITIZE_ADDRESS__) && !defined(__FILC__)
  check(delta <= 64L, "1000_frames RSS flat (<= 64 KB)");
#endif
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
  test_dl_refusals_and_parallel();
  test_dl_1000_frames();
  test_dl_budget_refusal();
  printf("c1_test failures=%d\n", g_fails);
  return g_fails == 0 ? 0 : 1;
}
