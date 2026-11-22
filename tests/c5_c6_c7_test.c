#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include "door.h"
#include "extract.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                           \
  do {                                                                             \
    if (!(cond)) {                                                                 \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));              \
      g_failures++;                                                                \
    }                                                                              \
  } while (0)

static bool has_reject_code(const StigiDoorResult *r, const char *code) {
  for (uint32_t i = 0; i < r->reject_count; i++) {
    if (strcmp(r->rejects[i].code, code) == 0) return true;
  }
  return false;
}

static void dump_rejects(const char *label, const StigiDoorResult *r) {
  for (uint32_t i = 0; i < r->reject_count; i++) {
    fprintf(stderr, "  [%s] reject: subj=%s code=%s msg=%s\n",
            label, r->rejects[i].subject, r->rejects[i].code, r->rejects[i].msg);
  }
}

/* Count non-header/non-border data rows in `tupu -D -` output for `reject` */
static int count_tupu_reject_rows(const char *out_buf) {
  const char *p = strstr(out_buf, "reject");
  if (p == NULL) return 0;
  /* Advance to first '===' separator line */
  p = strstr(p, "===");
  if (p == NULL) return 0;
  while (*p != '\0' && *p != '\n') p++;
  if (*p == '\n') p++;
  int rows = 0;
  while (*p != '\0') {
    const char *line = p;
    while (*p != '\0' && *p != '\n') p++;
    size_t len = (size_t)(p - line);
    if (*p == '\n') p++;
    if (len >= 3 && strncmp(line, "===", 3) == 0) break;
    if (len > 0 && line[0] != '-') rows++;
  }
  return rows;
}

int main(void) {
  StigiDoor door;
  Diag d;
  diag_clear(&d);

  Status st = stigi_door_init(&door, "vic/spec/datalog/all.dl",
                              STIGI_DEFAULT_ROW_BUDGET, true, &d);
  if (st != STIGI_OK) {
    fprintf(stderr, "FAIL stigi_door_init: %s\n", d.msg);
    return 1;
  }

  /* 1. Positive unit admission: guarded array read (main is always an allowed pub root) */
  const char *good_at =
      "// prompt: return element at a proved index.\n"
      "pub def main(count: U64, items: Array[U64](count), index: U64) <below(index, count)>: U64 =\n"
      "  items(index)\n";
  {
    const char *srcs[1] = {good_at};
    size_t lens[1] = {strlen(good_at)};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_submit(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "submit good_at");
    if (!r.accepted) dump_rejects("good_at", &r);
    CHECK(r.accepted && r.reject_count == 0, "guarded array read accepted with 0 rejects");
    CHECK(door.image.unit_count == 1, "live image has 1 unit");
    stigi_door_result_free(&r);
  }

  /* 2. Reprint oracle: two extractions of the same unit are byte-equal */
  {
    StigiFactSet fs1, fs2;
    stigi_facts_init(&fs1);
    stigi_facts_init(&fs2);
    CHECK(stigi_extract_unit(&door.image, 0, &fs1, &d) == STIGI_OK, "extract 1");
    CHECK(stigi_extract_unit(&door.image, 0, &fs2, &d) == STIGI_OK, "extract 2");
    Arena pa;
    arena_init(&pa);
    char *p1 = NULL, *p2 = NULL;
    size_t l1 = 0, l2 = 0;
    CHECK(stigi_facts_print(&fs1, &p1, &l1, &pa, &d) == STIGI_OK, "print 1");
    CHECK(stigi_facts_print(&fs2, &p2, &l2, &pa, &d) == STIGI_OK, "print 2");
    CHECK(l1 > 0 && l1 == l2 && memcmp(p1, p2, l1) == 0, "reprint oracle byte-equal");
    arena_free(&pa);
    stigi_facts_free(&fs1);
    stigi_facts_free(&fs2);
  }

  /* 3. Negative unit rejection: unguarded items(index) -> index-unproved, image unchanged */
  const char *bad_at =
      "// prompt: unguarded array read.\n"
      "pub def main(count: U64, items: Array[U64](count), index: U64) <lessOrEqual(0, count)>: U64 =\n"
      "  items(index)\n";
  {
    const char *srcs[1] = {bad_at};
    size_t lens[1] = {strlen(bad_at)};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_submit(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "submit bad_at");
    CHECK(!r.accepted && has_reject_code(&r, "index-unproved"),
          "unguarded read rejected with index-unproved");
    CHECK(door.image.unit_count == 1, "live image unchanged after rejection");
    stigi_door_result_free(&r);
  }

  /* 4. Atomic transaction rollback (T0): 1 good + 1 bad -> NEITHER admitted & cache untouched */
  const char *good_two =
      "// prompt: second guarded array read.\n"
      "val Demo.Tests.passOne {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(1, 1, \"one equals one\")\n";
  {
    uint32_t vcache_before = door.verdict_cache.count;
    uint32_t tcache_before = door.test_cache.count;
    const char *srcs[2] = {good_two, bad_at};
    size_t lens[2] = {strlen(good_two), strlen(bad_at)};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_submit(&door, srcs, lens, 2, &r, &d) == STIGI_OK, "submit atomic batch");
    CHECK(!r.accepted, "batch with 1 good + 1 bad rejected");
    CHECK(door.image.unit_count == 1, "atomic rollback: neither unit admitted");
    CHECK(door.verdict_cache.count == vcache_before && door.test_cache.count == tcache_before,
          "Fix #1: cache untouched on rollback");
    stigi_door_result_free(&r);
  }

  /* 5. Incremental cache: re-submitting with renamed parameter hits verdict cache (0 computed) */
  const char *good_at_renamed =
      "// prompt: return element at a proved index with renamed parameter.\n"
      "pub def main(limit: U64, items: Array[U64](limit), index: U64) <below(index, limit)>: U64 =\n"
      "  items(index)\n";
  {
    const char *srcs[1] = {good_at_renamed};
    size_t lens[1] = {strlen(good_at_renamed)};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_submit(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "submit renamed");
    if (!r.accepted) dump_rejects("renamed", &r);
    CHECK(r.accepted && r.verdicts_computed == 0 && r.verdicts_cached == 1,
          "alpha-equivalent resubmission hits verdict cache (0 computed, 1 cached)");
    stigi_door_result_free(&r);
  }

  /* 6. NegTest admission: passing, negtest-not-rejected, negtest-wrong-code */
  {
    const char *nt_pass =
        "// prompt: unguarded index must be rejected.\n"
        "val Demo.NegTests.indexWithoutBound {Stigi.NegTest}: Never =\n"
        "  def probe(count: U64, table: Array[U64](count), index: U64) <lessOrEqual(0, count)>: U64 = table(index)\n"
        "  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, \"no fact bounds the index\")\n";
    const char *srcs[1] = {nt_pass};
    size_t lens[1] = {strlen(nt_pass)};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_submit(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "submit nt_pass");
    if (!r.accepted) dump_rejects("nt_pass", &r);
    CHECK(r.accepted, "passing NegTest admitted");
    stigi_door_result_free(&r);

    const char *nt_not_rej =
        "// prompt: valid body should fail NegTest.\n"
        "val Demo.NegTests.shouldFail {Stigi.NegTest}: Never =\n"
        "  def probe(count: U64, table: Array[U64](count), index: U64) <below(index, count)>: U64 = table(index)\n"
        "  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, \"actually proved\")\n";
    srcs[0] = nt_not_rej;
    lens[0] = strlen(nt_not_rej);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check nt_not_rej");
    CHECK(!r.accepted && has_reject_code(&r, "negtest-not-rejected"),
          "NegTest whose body succeeds gives negtest-not-rejected");
    stigi_door_result_free(&r);

    const char *nt_wrong_code =
        "// prompt: wrong expected code.\n"
        "val Demo.NegTests.wrongCode {Stigi.NegTest}: Never =\n"
        "  def probe(count: U64, table: Array[U64](count), index: U64) <lessOrEqual(0, count)>: U64 = table(index)\n"
        "  Stigi.NegTest.expect(Stigi.Code.OverflowUnproved, \"wrong code\")\n";
    srcs[0] = nt_wrong_code;
    lens[0] = strlen(nt_wrong_code);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check nt_wrong_code");
    CHECK(!r.accepted && has_reject_code(&r, "negtest-wrong-code"),
          "NegTest with wrong expected code gives negtest-wrong-code");
    stigi_door_result_free(&r);
  }

  /* 7. Ruling S38: lowering rowBudget to 0 or 5 is rejected with datalog-budget */
  {
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_set_budget(&door, 0ULL, &r, &d) == STIGI_OK, "set_budget 0");
    CHECK(!r.accepted && has_reject_code(&r, "datalog-budget"),
          "Fix #7: rowBudget == 0 rejected with datalog-budget");
    stigi_door_result_free(&r);

    stigi_door_result_init(&r);
    CHECK(stigi_door_set_budget(&door, 5ULL, &r, &d) == STIGI_OK, "set_budget 5");
    CHECK(!r.accepted && has_reject_code(&r, "datalog-budget"),
          "lowering rowBudget to 5 rejected with datalog-budget");
    CHECK(door.row_budget == STIGI_DEFAULT_ROW_BUDGET, "row_budget unchanged on rejection");
    stigi_door_result_free(&r);
  }

  /* 8. C5-C7 adversarial review regression tests */
  {
    /* Fix #3: user-mode Stigi.* rejected reserved-name */
    StigiDoor user_door;
    CHECK(stigi_door_init(&user_door, "vic/spec/datalog/all.dl",
                          STIGI_DEFAULT_ROW_BUDGET, false, &d) == STIGI_OK,
          "init user_door");
    const char *stigi_unit =
        "// prompt: illegal user unit in Stigi namespace.\n"
        "val Stigi.Tests.illegal {Stigi.Test}: Stigi.Test.Result =\n"
        "  Stigi.Test.expect(1, 1, \"one equals one\")\n";
    const char *srcs[1] = {stigi_unit};
    size_t lens[1] = {strlen(stigi_unit)};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&user_door, srcs, lens, 1, &r, &d) == STIGI_OK, "check user Stigi.*");
    CHECK(!r.accepted && has_reject_code(&r, "reserved-name"),
          "Fix #3: user-mode Stigi.* rejected with reserved-name");
    stigi_door_result_free(&r);
    stigi_door_free(&user_door);

    /* Fix #4: pub-unused enforced */
    const char *unused_pub =
        "// prompt: unused pub def.\n"
        "pub def Demo.unusedPub(value: U64) <lessOrEqual(0, value)>: U64 =\n"
        "  value\n";
    srcs[0] = unused_pub;
    lens[0] = strlen(unused_pub);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check pub-unused");
    CHECK(!r.accepted && has_reject_code(&r, "pub-unused"),
          "Fix #4: unused pub def rejected with pub-unused");
    stigi_door_result_free(&r);

    /* Fix #11: var x@<below(x, count)> = index rejected assertion-unproved */
    const char *bad_var_inv =
        "// prompt: unproved var invariant.\n"
        "pub def main(count: U64, index: U64) <lessOrEqual(0, count)>: U64 =\n"
        "  var cur@<below(cur, count)>: U64 = index\n"
        "  cur\n";
    srcs[0] = bad_var_inv;
    lens[0] = strlen(bad_var_inv);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check bad_var_inv");
    CHECK(!r.accepted && has_reject_code(&r, "assertion-unproved"),
          "Fix #11: var x@<below(x, n)> = index rejected with assertion-unproved");
    stigi_door_result_free(&r);

    /* Fix #10: U8 overflow rejected overflow-unproved */
    const char *u8_ov =
        "// prompt: unguarded U8 addition.\n"
        "pub def main(left: U8, right: U8) <lessOrEqual(0, left)>: U8 =\n"
        "  left + right\n";
    srcs[0] = u8_ov;
    lens[0] = strlen(u8_ov);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check u8_ov");
    CHECK(!r.accepted && has_reject_code(&r, "overflow-unproved"),
          "Fix #10: U8 addition overflow rejected with overflow-unproved");
    stigi_door_result_free(&r);

    /* Fix #15: while without decreases in lemma def rejected lemma-unfounded */
    const char *bad_lemma =
        "// prompt: lemma with unfunded while loop.\n"
        "val Demo.NegTests.lemmaLoop {Stigi.NegTest}: Never =\n"
        "  lemma def spin(value: U64) <lessOrEqual(0, value)>: <lessOrEqual(0, value)> =\n"
        "    while true do\n"
        "      @<lessOrEqual(0, value)>\n"
        "  Stigi.NegTest.expect(Stigi.Code.LemmaUnfounded, \"while without decreases\")\n";
    srcs[0] = bad_lemma;
    lens[0] = strlen(bad_lemma);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check bad_lemma");
    if (!r.accepted) dump_rejects("bad_lemma", &r);
    CHECK(r.accepted, "Fix #15: while without decreases in lemma def caught as lemma-unfounded");
    stigi_door_result_free(&r);
  }

  /* 9. C7 export parity: export to /tmp/stigi-c7-export, run tupu CLI, assert 0 reject rows */
  {
    (void)mkdir("/tmp/stigi-c7-export", 0755);
    CHECK(stigi_door_export(&door, "/tmp/stigi-c7-export/all.dl",
                            "/tmp/stigi-c7-export", &d) == STIGI_OK,
          "stigi_door_export");
    FILE *fp = popen("/home/vic/hk/tupu/result/bin/tupu -F /tmp/stigi-c7-export -D - /tmp/stigi-c7-export/all.dl", "r");
    CHECK(fp != NULL, "popen tupu CLI on exported facts");
    if (fp != NULL) {
      char out_buf[8192];
      size_t n = fread(out_buf, 1, sizeof(out_buf) - 1, fp);
      out_buf[n] = '\0';
      int rc = pclose(fp);
      CHECK(rc == 0, "tupu CLI exited 0 on exported program");
      int rej_rows = count_tupu_reject_rows(out_buf);
      if (rej_rows != 0) {
        fprintf(stderr, "FAIL tupu export had %d reject rows:\n%s\n", rej_rows, out_buf);
      }
      CHECK(rej_rows == 0, "Fix #8: tupu CLI reject relation has exactly 0 rows");
    }
  }

  stigi_door_free(&door);

  if (g_failures == 0) {
    printf("c5_c6_c7_test: ALL PASS\n");
    return 0;
  }
  fprintf(stderr, "c5_c6_c7_test: %d FAILURES\n", g_failures);
  return 1;
}
