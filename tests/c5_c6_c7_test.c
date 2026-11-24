#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include "door.h"
#include "extract.h"
#include "names.h"

#include <dirent.h>
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

/* Count data rows in `tupu -D -` output for `relation reject` (also handles Souffle borders) */
static int count_tupu_reject_rows(const char *out_buf) {
  if (out_buf == NULL) return -1;
  const char *p = out_buf;
  while (*p != '\0') {
    const char *line = p;
    while (*p != '\0' && *p != '\n') p++;
    size_t len = (size_t)(p - line);
    if (*p == '\n') p++;
    if (len == 15 && memcmp(line, "relation reject", 15) == 0) {
      int rows = 0;
      while (*p != '\0') {
        const char *rline = p;
        while (*p != '\0' && *p != '\n') p++;
        size_t rlen = (size_t)(p - rline);
        if (*p == '\n') p++;
        if (rlen >= 9 && memcmp(rline, "relation ", 9) == 0) break;
        if (rlen > 0) rows++;
      }
      return rows;
    }
  }
  return -1; /* Missing `relation reject` header: fail closed! */
}

/* Reject rows of `tupu` on one exported fact directory (-1: tupu failed or printed no reject relation) */
static int tupu_reject_rows(const char *tupu_bin, const char *root, const char *dir) {
  char cmd[2048];
  (void)snprintf(cmd, sizeof cmd, "%s -F %s -D - %s/rules.dl", tupu_bin, dir, root);
  FILE *fp = popen(cmd, "r");
  if (fp == NULL) return -1;
  char out_buf[8192];
  size_t n = fread(out_buf, 1, sizeof(out_buf) - 1, fp);
  out_buf[n] = '\0';
  if (pclose(fp) != 0) return -1;
  return count_tupu_reject_rows(out_buf);
}

static bool write_file_text(const char *path, const char *text) {
  FILE *f = fopen(path, "wb");
  if (f == NULL) return false;
  size_t n = strlen(text);
  bool ok = fwrite(text, 1, n, f) == n;
  return fclose(f) == 0 && ok;
}

/* One fact directory for `prog`: an empty file for every .input relation, then the given files. */
static bool write_fact_dir(const char *dir, const char *prog, const char *const *files, uint32_t nfiles) {
  if (mkdir(dir, 0700) != 0) return false;
  for (const char *p = strstr(prog, "\n.input "); p != NULL; p = strstr(p + 1, "\n.input ")) {
    const char *q = p + 8;
    while (*q != '\0' && *q != '\n') {
      while (*q == ' ' || *q == ',') q++;
      const char *s = q;
      while (*q != '\0' && *q != '\n' && *q != ',' && *q != ' ' && *q != '(') q++;
      if (q > s) {
        char path[2048];
        (void)snprintf(path, sizeof path, "%s/%.*s.facts", dir, (int)(q - s), s);
        if (!write_file_text(path, "")) return false;
      }
      while (*q != '\0' && *q != '\n' && *q != ',') q++;
    }
  }
  for (uint32_t i = 0; i + 1U < nfiles; i += 2U) {
    char path[2048];
    (void)snprintf(path, sizeof path, "%s/%s.facts", dir, files[i]);
    if (!write_file_text(path, files[i + 1U])) return false;
  }
  return true;
}

/* Rows of Stigi.Probe.reachAt from tupu on `dir`, one "value\tlimit\tat" per line in `out` (-1 on failure) */
static int reach_rows(const char *tupu_bin, const char *prog_path, const char *dir, char *out, size_t cap) {
  char odir[1200], cmd[4096], csv[1300];
  (void)snprintf(odir, sizeof odir, "%s.out", dir);
  if (mkdir(odir, 0700) != 0) return -1;
  (void)snprintf(cmd, sizeof cmd, "%s -F %s -D %s %s > /dev/null 2>&1", tupu_bin, dir, odir, prog_path);
  if (system(cmd) != 0) return -1;
  (void)snprintf(csv, sizeof csv, "%s/Stigi__Probe__reachAt.csv", odir);
  FILE *f = fopen(csv, "rb");
  if (f == NULL) return -1;
  size_t n = fread(out, 1, cap - 1U, f);
  out[n] = '\0';
  (void)fclose(f);
  int rows = 0;
  for (size_t i = 0; i < n; i++) rows += out[i] == '\n';
  return rows;
}

/* S129 sugar refusals: each candidate is rejected with exactly one code, the one under test (T2: a NegTest needs
 * exactly its codes) */
static void test_s129_negatives(StigiDoor *door) {
  int failures_before = g_failures;
  const char *shape =
      "// prompt: a drawn shape.\n"
      "pub enum Demo.Neg.Shape:\n"
      "  case Dot\n"
      "  case Line(len: U64)\n"
      "  case Box(wide: U64, high: U64)\n";
  struct {
    const char *label;
    const char *code;
    const char *src;
  } cases[3] = {
      {"S129: a range whose low end is above its high end is mismatch", "mismatch",
       "// prompt: a reversed range must be refused.\n"
       "pub def main(code: U8): I32 <lessOrEqual(0, result)> =\n"
       "  code match\n"
       "    case 9..0 => 1\n"
       "    case _ => 0\n"},
      {"S129: alternatives with other binders are pattern-arity", "pattern-arity",
       "// prompt: alternatives with different binders must be refused.\n"
       "pub def main(shape: Demo.Neg.Shape): U64 <lessOrEqual(0, result)> =\n"
       "  shape match\n"
       "    case Line(len) | Dot => len\n"
       "    case Box(_, _) => 0\n"},
      {"S129: a text match without a last default arm is not-exhaustive", "not-exhaustive",
       "// prompt: a text match without a default arm must be refused.\n"
       "pub def main(count: U64, word: Array[U8](count)): I32 <lessOrEqual(0, result)> =\n"
       "  word match\n"
       "    case \"yes\" => 1\n"}};
  for (int k = 0; k < 3; k++) {
    /* only the binder case reads the shape enum; an unread enum would add no-reader */
    uint32_t n = k == 1 ? 2U : 1U;
    const char *srcs[2] = {k == 1 ? shape : cases[k].src, cases[k].src};
    size_t lens[2] = {strlen(srcs[0]), strlen(srcs[1])};
    Diag d;
    diag_clear(&d);
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(door, srcs, lens, n, &r, &d) == STIGI_OK, "check S129 refusal candidate");
    bool exact = !r.accepted && r.reject_count == 1 && strcmp(r.rejects[0].code, cases[k].code) == 0;
    if (!exact) dump_rejects(cases[k].label, &r);
    CHECK(exact, cases[k].label);
    stigi_door_result_free(&r);
  }
  if (g_failures == failures_before) printf("c6 S129 refusals: PASS (mismatch, pattern-arity, not-exhaustive)\n");
}

/* S138 (Option A): an order atom in a rule body reads the kernel's stored row through dom; order atoms are refused
 * negated, in a head, and the P1 closure relations are never named by a rule unit. */
static void test_s138_rule_mapping(StigiDoor *door, const char *tupu_bin) {
  int failures_before = g_failures;
  Diag d;
  diag_clear(&d);
  const char *rel =
      "// prompt: scopes at which a value is below and at most its limit.\n"
      "pub relation Stigi.Probe.reachAt(value: Stigi.Id.Value, limit: Stigi.Id.Value, at: Stigi.Id.Value)\n";
  const char *rule =
      "// prompt: a value that is below and at most its limit at a scope reaches it there.\n"
      "rule def Stigi.Probe.reachRule: Stigi.Probe.reachAt(value, limit, at) = below(value, limit, at), "
      "lessOrEqual(value, limit, at)\n";
  StigiImage img;
  stigi_image_init(&img);
  StigiNameDiags nd;
  stigi_name_diags_init(&nd);
  CHECK(stigi_image_add_source(&img, rel, strlen(rel), &nd, &d) == STIGI_OK &&
            stigi_image_add_source(&img, rule, strlen(rule), &nd, &d) == STIGI_OK &&
            stigi_image_resolve(&img, &nd, &d) == STIGI_OK,
        "S138 candidate with a relation and a rule unit");
  char *dl = NULL;
  size_t dl_len = 0;
  bool custom = false;
  CHECK(stigi_door_compile_rules(door, &img, &dl, &dl_len, &custom, &d) == STIGI_OK && custom && dl != NULL,
        "S138 candidate rules compile");
  stigi_name_diags_free(&nd);
  stigi_image_free(&img);
  if (dl == NULL) return;
  CHECK(strstr(dl, "belowFrom(value, limit, _dU0), dom(at, _dU0)") != NULL,
        "S138: below(a, b, s) reads belowFrom(a, b, _dU0), dom(s, _dU0)");
  CHECK(strstr(dl, "leqFrom(value, limit, _dU1), dom(at, _dU1)") != NULL,
        "S138: lessOrEqual(a, b, s) reads leqFrom(a, b, _dU1), dom(s, _dU1)");

  /* tupu accepts the program; a credit at the root is seen at the child, a credit at the child is not seen at the
   * root (scopes R and C, C child of R) */
  char base[] = "/tmp/stigi-s138-rule-XXXXXX";
  CHECK(mkdtemp(base) != NULL, "mkdtemp S138 rule dir");
  char prog_path[1024];
  (void)snprintf(prog_path, sizeof prog_path, "%s/prog.dl", base);
  size_t plen = dl_len + 64U;
  char *prog = malloc(plen);
  CHECK(prog != NULL, "S138 program buffer");
  if (prog == NULL) {
    free(dl);
    return;
  }
  (void)snprintf(prog, plen, "%s\n.output Stigi__Probe__reachAt\n", dl);
  free(dl);
  CHECK(write_file_text(prog_path, prog), "write S138 program");
  const char *tree[] = {"scope", "R\nC\n", "parent", "C\tR\n", "scopeSpan", "R\t0\t1\nC\t1\t1\n", "value", "v\nl\n"};
  const char *at_root[] = {"baseBelow", "v\tl\tR\n", "baseLeq", "v\tl\tR\n"};
  const char *at_child[] = {"baseBelow", "v\tl\tC\n", "baseLeq", "v\tl\tC\n"};
  const char *const *credit[2] = {at_root, at_child};
  const char *want[2][2] = {{"v\tl\tR\n", "v\tl\tC\n"}, {"v\tl\tC\n", NULL}};
  const int want_rows[2] = {2, 1};
  for (int c = 0; c < 2; c++) {
    char dir[1100];
    (void)snprintf(dir, sizeof dir, "%s/%s", base, c == 0 ? "root" : "child");
    const char *files[12];
    for (int k = 0; k < 8; k++) files[k] = tree[k];
    for (int k = 0; k < 4; k++) files[8 + k] = credit[c][k];
    CHECK(write_fact_dir(dir, prog, files, 12), "S138 two-scope fact dir");
    char rows[512];
    int n = reach_rows(tupu_bin, prog_path, dir, rows, sizeof rows);
    CHECK(n == want_rows[c], c == 0 ? "S138: a root credit gives reachAt at R and at C"
                                    : "S138: a child credit gives reachAt at C only");
    for (int k = 0; k < 2; k++) {
      if (want[c][k] != NULL) CHECK(strstr(rows, want[c][k]) != NULL, "S138: expected reachAt row present");
    }
  }
  free(prog);

  /* Refusals (rule-unsafe): a negated order atom, an order alias in a head, a body atom on a P1 closure relation */
  const char *seen =
      "// prompt: scopes at which a value was seen with its limit.\n"
      "pub relation Stigi.Probe.seen(value: Stigi.Id.Value, limit: Stigi.Id.Value, at: Stigi.Id.Value)\n";
  const char *bad_rules[3] = {
      "// prompt: a negated order atom must be refused.\n"
      "rule def Stigi.Probe.negRule: Stigi.Probe.reachAt(value, limit, at) = Stigi.Probe.seen(value, limit, at), "
      "not below(value, limit, at)\n",
      "// prompt: an order alias in a head must be refused.\n"
      "rule def Stigi.Probe.headRule: below(value, limit, at) = Stigi.Probe.seen(value, limit, at)\n",
      "// prompt: a P1 closure relation in a body must be refused.\n"
      "rule def Stigi.Probe.innerRule: Stigi.Probe.reachAt(value, limit, at) = belowFrom(value, limit, at)\n"};
  const char *labels[3] = {"S138: a negated order atom is rule-unsafe", "S138: an order alias in a head is rule-unsafe",
                           "S138: a P1 closure relation in a body is rule-unsafe"};
  for (int k = 0; k < 3; k++) {
    const char *srcs[3] = {rel, seen, bad_rules[k]};
    size_t lens[3] = {strlen(rel), strlen(seen), strlen(bad_rules[k])};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(door, srcs, lens, 3, &r, &d) == STIGI_OK, "check S138 refusal candidate");
    if (!has_reject_code(&r, "rule-unsafe")) dump_rejects(labels[k], &r);
    CHECK(!r.accepted && has_reject_code(&r, "rule-unsafe"), labels[k]);
    stigi_door_result_free(&r);
  }
  if (g_failures == failures_before) printf("c7 S138 rule mapping: PASS (dom reads, two-scope rows, 3 refusals)\n");
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

  /* 9. Wave 3 hardening: 128-bit U64/I64, SSA/while/D1 units, stack, boundaries, custom rules */
  {
    /* Armed self-test of count_tupu_reject_rows */
    CHECK(count_tupu_reject_rows("relation reject\nrelation freeEdge\n") == 0,
          "count_tupu_reject_rows 0 rows");
    CHECK(count_tupu_reject_rows("relation reject\nfoo\tindex-unproved\nrelation freeEdge\n") == 1,
          "count_tupu_reject_rows armed 1 row");
    CHECK(count_tupu_reject_rows("no header here") == -1,
          "count_tupu_reject_rows fail-closed on missing header");

    /* 9a. Exact 128-bit U64 max & guarded addition + I64 min */
    const char *u64_max_ok =
        "// prompt: U64 literal 2^64-1 and guarded increment.\n"
        "pub def main(value: U64) <below(value, 18446744073709551615)>: U64 =\n"
        "  value + 1\n";
    const char *srcs[6];
    size_t lens[6];
    srcs[0] = u64_max_ok;
    lens[0] = strlen(u64_max_ok);
    StigiDoorResult r;
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check u64_max_ok");
    if (!r.accepted) dump_rejects("u64_max_ok", &r);
    CHECK(r.accepted, "Wave 3: U64 value < 2^64-1 allows value + 1");
    stigi_door_result_free(&r);

    const char *u64_unproved_sub =
        "// prompt: unproved U64 subtraction underflow.\n"
        "pub def main(left: U64, right: U64) <lessOrEqual(0, left)>: U64 =\n"
        "  left - right\n";
    srcs[0] = u64_unproved_sub;
    lens[0] = strlen(u64_unproved_sub);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check u64_sub");
    CHECK(!r.accepted && has_reject_code(&r, "overflow-unproved"),
          "Wave 3: unproved U64 subtraction rejected with overflow-unproved");
    stigi_door_result_free(&r);

    const char *i64_min_ok =
        "// prompt: exact I64 minimum literal.\n"
        "pub def main(value: I64): I64 <lessOrEqual(-9223372036854775808, result)> =\n"
        "  if value < 0 then -9223372036854775808 else value\n";
    srcs[0] = i64_min_ok;
    lens[0] = strlen(i64_min_ok);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check i64_min_ok");
    if (!r.accepted) dump_rejects("i64_min_ok", &r);
    CHECK(r.accepted, "Wave 3: exact I64 minimum literal -9223372036854775808 accepted");
    stigi_door_result_free(&r);

    /* 9b. Local val alias array read: unguarded MUST fail index-unproved, guarded passes */
    const char *local_arr_bad =
        "// prompt: unguarded read through local val array alias.\n"
        "pub def main(count: U64, items: Array[U64](count), index: U64) <lessOrEqual(0, count)>: U64 =\n"
        "  val copy: Array[U64](count) = items\n"
        "  copy(index)\n";
    srcs[0] = local_arr_bad;
    lens[0] = strlen(local_arr_bad);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check local_arr_bad");
    CHECK(!r.accepted && has_reject_code(&r, "index-unproved"),
          "Wave 3: unguarded read on local val array rejected with index-unproved");
    stigi_door_result_free(&r);

    /* 9c. While invariant back-edge check: pos = pos + 2 violates pos <= len */
    const char *while_bad_inv =
        "// prompt: while loop whose step breaks invariant pos <= len.\n"
        "pub def main(len: U64, start: U64) <lessOrEqual(start, len)>: U64 =\n"
        "  var pos: U64 = start\n"
        "  while pos < len do\n"
        "    @<lessOrEqual(start, pos), lessOrEqual(pos, len)>\n"
        "    pos = pos + 2\n"
        "  pos\n";
    srcs[0] = while_bad_inv;
    lens[0] = strlen(while_bad_inv);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check while_bad_inv");
    CHECK(!r.accepted && (has_reject_code(&r, "invariant-unproved") ||
                          has_reject_code(&r, "overflow-unproved")),
          "Wave 3: while loop violating invariant on back edge rejected");
    stigi_door_result_free(&r);

    /* 9d. Real D1 unit: Stigi.Lex.step (while + && + invariant + decreases(len - pos)) */
    const char *d1_is_ident =
        "// prompt: return true when byte is ASCII identifier byte.\n"
        "def Stigi.Lex.isIdentByte(byte: U8): Bool =\n"
        "  (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') || (byte >= '0' && byte <= '9') || byte == '_' || byte == '.'\n";
    const char *d1_lex_step =
        "// prompt: scan one identifier span forward in source starting at start.\n"
        "pub def Stigi.Lex.step(len: U64, start: U64, source: Array[U8](len)) <lessOrEqual(start, len)>: U64 <lessOrEqual(start, result), lessOrEqual(result, len)> =\n"
        "  var pos: U64 = start\n"
        "  while pos < len && Stigi.Lex.isIdentByte(source(pos)) do\n"
        "    @<lessOrEqual(start, pos), lessOrEqual(pos, len), decreases(len - pos)>\n"
        "    pos = pos + 1\n"
        "  pos\n";
    const char *d1_lex_main =
        "// prompt: caller root for Stigi.Lex.step.\n"
        "pub def main(len: U64, source: Array[U8](len)) <lessOrEqual(0, len)>: U64 =\n"
        "  Stigi.Lex.step(len, 0, source)\n";
    srcs[0] = d1_is_ident; lens[0] = strlen(d1_is_ident);
    srcs[1] = d1_lex_step; lens[1] = strlen(d1_lex_step);
    srcs[2] = d1_lex_main; lens[2] = strlen(d1_lex_main);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 3, &r, &d) == STIGI_OK, "check D1 Lex.step");
    if (!r.accepted) dump_rejects("D1_Lex_step", &r);
    CHECK(r.accepted, "Wave 3: real D1 unit Stigi.Lex.step accepted with 0 rejects");
    stigi_door_result_free(&r);

    /* 9e. Real D1 unit: Stigi.Arena.tryAllocate (mod/sub/add + phi + Array.tryFill) */
    const char *d1_max_chunk =
        "// prompt: maximum arena chunk size.\n"
        "pub val Stigi.Arena.maxChunk: U64 = 0x4000000000000000\n";
    const char *d1_alloc_res =
        "// prompt: outcome of aligned arena bump allocation.\n"
        "pub enum Stigi.Arena.AllocResult:\n"
        "  case Allocated(offset: U64, nextPos: U64, cap: U64, chunk: Array[U8](cap)) <lessOrEqual(offset, nextPos), lessOrEqual(nextPos, cap)>\n"
        "  case FillFailed\n";
    const char *d1_arena_try =
        "// prompt: allocate bytes with 8-byte alignment from an arena chunk.\n"
        "pub def Stigi.Arena.tryAllocate {region: Stigi.Region} (cap: U64, pos: U64, bytes: U64, chunk: Array[U8](cap)) <lessOrEqual(pos, cap), below(cap, Stigi.Arena.maxChunk), below(bytes, Stigi.Arena.maxChunk)>: Stigi.Arena.AllocResult =\n"
        "  val rem: U64 = bytes % 8\n"
        "  val pad: U64 = (8 - rem) % 8\n"
        "  val aligned: U64 = bytes + pad\n"
        "  val nextPos: U64 = pos + aligned\n"
        "  if nextPos <= cap then\n"
        "    AllocResult.Allocated(pos, nextPos, cap, chunk)\n"
        "  else\n"
        "    val chunkCap: U64 = if aligned < 65536 then 65536 else aligned\n"
        "    Array.tryFill{region}(chunkCap, 0) match\n"
        "      case Filled(freshChunk) => AllocResult.Allocated(0, aligned, chunkCap, freshChunk)\n"
        "      case FillFailed => AllocResult.FillFailed\n";
    const char *d1_arena_main =
        "// prompt: caller root for Stigi.Arena.tryAllocate.\n"
        "pub def main {region: Stigi.Region} (cap: U64, chunk: Array[U8](cap)) <below(cap, Stigi.Arena.maxChunk)>: Stigi.Arena.AllocResult =\n"
        "  Stigi.Arena.tryAllocate(cap, 0, 24, chunk)\n";
    srcs[0] = d1_max_chunk; lens[0] = strlen(d1_max_chunk);
    srcs[1] = d1_alloc_res; lens[1] = strlen(d1_alloc_res);
    srcs[2] = d1_arena_try; lens[2] = strlen(d1_arena_try);
    srcs[3] = d1_arena_main; lens[3] = strlen(d1_arena_main);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 4, &r, &d) == STIGI_OK, "check D1 Arena.tryAllocate");
    if (!r.accepted) dump_rejects("D1_Arena", &r);
    CHECK(r.accepted, "Wave 3: real D1 unit Stigi.Arena.tryAllocate accepted with 0 rejects");
    stigi_door_result_free(&r);

    /* 9e2. Real D1 unit: Stigi.Table.append (generic [A: Type], maxCap, items(count) := row, Array.tryGrow) */
    const char *d1_tbl_max =
        "// prompt: maximum table capacity that doubles without overflowing U64.\n"
        "pub val Stigi.Table.maxCap: U64 = 0x4000000000000000\n";
    const char *d1_tbl_res =
        "// prompt: outcome of appending one row into a heap-backed table.\n"
        "pub enum Stigi.Table.AppendResult[A: Type]:\n"
        "  case Appended(cap: U64, count: U64, items: Array[A](cap)) <lessOrEqual(count, cap), below(0, cap)>\n"
        "  case AppendFailed(cap: U64, count: U64, items: Array[A](cap)) <lessOrEqual(count, cap), below(0, cap)>\n";
    const char *d1_tbl_append =
        "// prompt: append one row into a heap-backed table, growing by doubling via Array.tryGrow.\n"
        "pub def Stigi.Table.append [A: Type] {region: Stigi.Region, Stigi.Mut} (\n"
        "    cap: U64, count: U64, items: Array[A](cap), row: A\n"
        ") <lessOrEqual(count, cap), below(0, cap), below(cap, Stigi.Table.maxCap)>: Stigi.Table.AppendResult[A] =\n"
        "  if count < cap then\n"
        "    items(count) := row\n"
        "    AppendResult.Appended(cap, count + 1, items)\n"
        "  else\n"
        "    val nextCap: U64 = cap + cap\n"
        "    Array.tryGrow{region}(items, nextCap, row) match\n"
        "      case Grown(grown) =>\n"
        "        grown(count) := row\n"
        "        AppendResult.Appended(nextCap, count + 1, grown)\n"
        "      case GrowFailed(kept) =>\n"
        "        AppendResult.AppendFailed(cap, count, kept)\n";
    const char *d1_tbl_main =
        "// prompt: caller root for Stigi.Table.append.\n"
        "pub def main {region: Stigi.Region, Stigi.Mut} (cap: U64, items: Array[U64](cap)) <below(0, cap), below(cap, Stigi.Table.maxCap)>: Stigi.Table.AppendResult[U64] =\n"
        "  Stigi.Table.append(cap, 0, items, 42)\n";
    srcs[0] = d1_tbl_max; lens[0] = strlen(d1_tbl_max);
    srcs[1] = d1_tbl_res; lens[1] = strlen(d1_tbl_res);
    srcs[2] = d1_tbl_append; lens[2] = strlen(d1_tbl_append);
    srcs[3] = d1_tbl_main; lens[3] = strlen(d1_tbl_main);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 4, &r, &d) == STIGI_OK, "check D1 Table.append");
    if (!r.accepted) dump_rejects("D1_Table", &r);
    CHECK(r.accepted, "Wave 3: real D1 unit Stigi.Table.append accepted with 0 rejects");
    stigi_door_result_free(&r);

    /* 9f. Non-tail recursion rejected with stack-unbounded; bounded recursion accepted */
    const char *nontail_rec =
        "// prompt: non-tail recursion without depth bound.\n"
        "pub def main(limit: U64) <lessOrEqual(0, limit), below(limit, 1000)>: U64 =\n"
        "  if limit < 1 then 0 else 1 + main(limit - 1)\n";
    srcs[0] = nontail_rec;
    lens[0] = strlen(nontail_rec);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check nontail_rec");
    CHECK(!r.accepted && has_reject_code(&r, "stack-unbounded"),
          "Wave 3: non-tail recursion without depth bound rejected with stack-unbounded");
    stigi_door_result_free(&r);

    const char *bounded_rec =
        "// prompt: bounded non-tail recursion with decreases and upper limit.\n"
        "pub def main(limit: U64) <lessOrEqual(0, limit), below(limit, 16), decreases(limit)>: U64 <lessOrEqual(result, limit)> =\n"
        "  if limit < 1 then 0 else 1 + main(limit - 1)\n";
    srcs[0] = bounded_rec;
    lens[0] = strlen(bounded_rec);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 1, &r, &d) == STIGI_OK, "check bounded_rec");
    if (!r.accepted) dump_rejects("bounded_rec", &r);
    CHECK(r.accepted, "Wave 3: bounded non-tail recursion with decreases+below accepted");
    stigi_door_result_free(&r);

    /* 9g. Boundary checks: user-mode top-level-name & test-leaks-capability */
    StigiDoor udoor;
    CHECK(stigi_door_init(&udoor, "vic/spec/datalog/all.dl",
                          STIGI_DEFAULT_ROW_BUDGET, false, &d) == STIGI_OK,
          "init udoor");
    const char *top_name_u =
        "// prompt: illegal top-level non-main unit in user mode.\n"
        "val helper {Stigi.Test}: Stigi.Test.Result =\n"
        "  Stigi.Test.expect(1, 1, \"ok\")\n";
    srcs[0] = top_name_u;
    lens[0] = strlen(top_name_u);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&udoor, srcs, lens, 1, &r, &d) == STIGI_OK, "check top-level-name");
    CHECK(!r.accepted && has_reject_code(&r, "top-level-name"),
          "Wave 3: user-mode non-main top-level unit rejected with top-level-name");
    stigi_door_result_free(&r);
    stigi_door_free(&udoor);

    const char *leak_cap_test =
        "// prompt: test leaking Stigi.File capability.\n"
        "val Demo.Tests.leakCap {Stigi.Test, Stigi.File}: Stigi.Test.Result =\n"
        "  Stigi.Test.expect(1, 1, \"ok\")\n";
    const char *cap_file_u =
        "// prompt: file capability.\n"
        "pub capability Stigi.File:\n"
        "  def read(count: U64): U64\n";
    srcs[0] = cap_file_u; lens[0] = strlen(cap_file_u);
    srcs[1] = leak_cap_test; lens[1] = strlen(leak_cap_test);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 2, &r, &d) == STIGI_OK, "check leak_cap_test");
    CHECK(!r.accepted && has_reject_code(&r, "test-leaks-capability"),
          "Wave 3: Test with extra capability rejected with test-leaks-capability");
    stigi_door_result_free(&r);

    /* 9h. In-image custom relation & rule: valid accepted, safety-reads-user rejected */
    const char *user_rel =
        "// prompt: user relation for custom rule test.\n"
        "pub relation Demo.tagged(item: Stigi.Id.Value)\n";
    const char *user_rel2 =
        "// prompt: derived user relation.\n"
        "relation Demo.marked(item: Stigi.Id.Value)\n";
    const char *user_rule =
        "// prompt: derive marked from tagged.\n"
        "rule def Demo.markRule: Demo.marked(item) = Demo.tagged(item)\n";
    const char *user_rule_bad =
        "// prompt: illegal kernel rule reading user relation.\n"
        "rule def Stigi.badRule: Stigi.Gate.cleared(item, \"index-unproved\") = Demo.tagged(item)\n";
    srcs[0] = user_rel; lens[0] = strlen(user_rel);
    srcs[1] = user_rel2; lens[1] = strlen(user_rel2);
    srcs[2] = user_rule; lens[2] = strlen(user_rule);
    srcs[3] = user_rule_bad; lens[3] = strlen(user_rule_bad);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&door, srcs, lens, 4, &r, &d) == STIGI_OK, "check safety-reads-user");
    CHECK(!r.accepted && has_reject_code(&r, "safety-reads-user"),
          "Wave 3: kernel rule reading user relation rejected with safety-reads-user");
    stigi_door_result_free(&r);

    /* 9i. Pure drop tree-shakes private helper */
    StigiDoor ddoor;
    CHECK(stigi_door_init(&ddoor, "vic/spec/datalog/all.dl",
                          STIGI_DEFAULT_ROW_BUDGET, true, &d) == STIGI_OK,
          "init ddoor");
    const char *drop_h =
        "// prompt: private helper.\n"
        "def Demo.dropHelper(value: U64) <lessOrEqual(0, value)>: U64 = value\n";
    const char *drop_root =
        "// prompt: test root using helper.\n"
        "val Demo.Tests.dropRoot {Stigi.Test}: Stigi.Test.Result =\n"
        "  Stigi.Test.expect(Demo.dropHelper(7), 7, \"7 == 7\")\n";
    srcs[0] = drop_h; lens[0] = strlen(drop_h);
    srcs[1] = drop_root; lens[1] = strlen(drop_root);
    stigi_door_result_init(&r);
    CHECK(stigi_door_submit(&ddoor, srcs, lens, 2, &r, &d) == STIGI_OK && r.accepted,
          "submit root + private helper");
    stigi_door_result_free(&r);
    const char *drops[1] = {"Demo.Tests.dropRoot"};
    stigi_door_result_init(&r);
    CHECK(stigi_door_drop(&ddoor, drops, 1, &r, &d) == STIGI_OK, "drop root");
    if (!r.accepted) dump_rejects("drop_tree_shake", &r);
    CHECK(r.accepted && ddoor.image.unit_count == 0,
          "Wave 3: dropping root tree-shakes orphaned private helper");
    stigi_door_result_free(&r);
    stigi_door_free(&ddoor);
  }

  /* S142 (capabilities.md 4): a def receiving a stateful capability states its entry and exit with at(c, S), or with
   * no at fact preserves the state and may call only state-preserving operations; a call of an at def is a state
   * transition of the caller */
  {
    const char *pipe_cap =
        "// prompt: a pipe that is open until it is shut.\n"
        "pub capability Demo.Pipe:\n"
        "  state Open, Closed\n"
        "  start Open\n"
        "  stop Closed\n"
        "  def peek() <at(self, Open)>: Bool <at(self, Open)>\n"
        "  def shut() <at(self, Open)>: Bool <at(self, Closed)>\n";
    const char *pipe_handler =
        "// prompt: an in-memory pipe.\n"
        "pub handler Demo.MemPipe(ready: Bool) for Demo.Pipe:\n"
        "  def peek(): Bool = ready\n"
        "  def shut(): Bool = ready\n";
    const char *drain_ok =
        "// prompt: peek an open pipe after a bounded number of tail steps.\n"
        "pub def Demo.drain {pipe: Demo.Pipe} (limit: U64) <at(pipe, Open), decreases(limit)>: Bool <at(pipe, Open)> =\n"
        "  if limit < 1 then pipe.peek() else Demo.drain{pipe}(limit - 1)\n";
    const char *look_ok =
        "// prompt: peek an open pipe; with no exit state stated it leaves the pipe open.\n"
        "pub def Demo.look {pipe: Demo.Pipe} (limit: U64) <at(pipe, Open), below(limit, 10)>: Bool =\n"
        "  pipe.peek()\n";
    const char *look_no_state =
        "// prompt: peek a pipe with no state stated for it.\n"
        "pub def Demo.look {pipe: Demo.Pipe} (limit: U64) <below(limit, 10)>: Bool =\n"
        "  pipe.peek()\n";
    const char *main_ok =
        "// prompt: drain and look at a pipe a bounded number of steps, then shut it.\n"
        "pub def main(limit: U64) <below(limit, 10)>: Bool =\n"
        "  handle pipe = Demo.MemPipe(true)\n"
        "  val drained: Bool = Demo.drain{pipe}(limit)\n"
        "  val looked: Bool = Demo.look{pipe}(2)\n"
        "  pipe.shut() && drained && looked\n";
    const char *drain_unknown =
        "// prompt: a state name that the pipe does not have.\n"
        "pub def Demo.drain {pipe: Demo.Pipe} (limit: U64) <at(pipe, Opn), decreases(limit)>: Bool <at(pipe, Open)> =\n"
        "  if limit < 1 then pipe.peek() else Demo.drain{pipe}(limit - 1)\n";
    const char *drain_shuts =
        "// prompt: promise an open pipe on exit and shut it.\n"
        "pub def Demo.drain {pipe: Demo.Pipe} (limit: U64) <at(pipe, Open), below(limit, 10)>: Bool <at(pipe, Open)> =\n"
        "  pipe.shut()\n";
    const char *look_shuts =
        "// prompt: an entry-only open pipe def that shuts the pipe.\n"
        "pub def Demo.look {pipe: Demo.Pipe} (limit: U64) <at(pipe, Open), below(limit, 10)>: Bool =\n"
        "  pipe.shut()\n";
    const char *main_late =
        "// prompt: drain a pipe after it is shut.\n"
        "pub def main(): Bool =\n"
        "  handle pipe = Demo.MemPipe(true)\n"
        "  val looked: Bool = Demo.look{pipe}(2)\n"
        "  val shutFirst: Bool = pipe.shut()\n"
        "  shutFirst && looked && Demo.drain{pipe}(3)\n";
    struct { const char *label; const char *drain; const char *look; const char *main; const char *code; } cases[6] = {
        {"S142 at(c, Open) tail recursion and an entry-only def accepted", drain_ok, look_ok, main_ok, NULL},
        {"S142 unknown state name rejected with unknown-state", drain_unknown, look_ok, main_ok, "unknown-state"},
        {"S142 exit at(c, Open) after shut rejected with exit-unproved", drain_shuts, look_ok, main_ok, "exit-unproved"},
        {"S142 entry-only at(c, Open) def that shuts rejected with exit-unproved", drain_ok, look_shuts, main_ok,
         "exit-unproved"},
        {"S142 def with no state for its pipe calling peek rejected with op-not-allowed", drain_ok, look_no_state,
         main_ok, "op-not-allowed"},
        {"S142 caller draining a shut pipe rejected with op-not-allowed", drain_ok, look_ok, main_late,
         "op-not-allowed"},
    };
    for (int k = 0; k < 6; k++) {
      const char *psrcs[5] = {pipe_cap, pipe_handler, cases[k].drain, cases[k].look, cases[k].main};
      size_t plens[5];
      for (int i = 0; i < 5; i++) plens[i] = strlen(psrcs[i]);
      StigiDoorResult pr;
      stigi_door_result_init(&pr);
      CHECK(stigi_door_check(&door, psrcs, plens, 5, &pr, &d) == STIGI_OK, cases[k].label);
      bool want_ok = cases[k].code == NULL;
      bool got = want_ok ? pr.accepted : (!pr.accepted && has_reject_code(&pr, cases[k].code));
      if (!got) dump_rejects(cases[k].label, &pr);
      CHECK(got, cases[k].label);
      stigi_door_result_free(&pr);
    }
  }

  /* 9h. Phase 2.1 oracles: fact-idle, (A || B) && C, val Ctor, nominal type T = U <facts>, protocol-only sigs */
  {
    StigiDoor p2door;
    CHECK(stigi_door_init(&p2door, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &d) == STIGI_OK,
          "init p2door");
    const char *p2s[6];
    size_t p2l[6];
    StigiDoorResult r;

    /* (a) fact-idle rejects vacuous <lessOrEqual(0, 1)> */
    const char *vac_bad =
        "// prompt: vacuous fact must be rejected.\n"
        "pub def main(value: U64) <lessOrEqual(0, 1)>: U64 = value\n";
    p2s[0] = vac_bad; p2l[0] = strlen(vac_bad);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p2s, p2l, 1, &r, &d) == STIGI_OK, "check vac_bad");
    CHECK(!r.accepted && has_reject_code(&r, "fact-idle"),
          "Phase 2.1: vacuous <lessOrEqual(0, 1)> rejected with fact-idle");
    stigi_door_result_free(&r);

    /* (b) (A || B) && C credits C into then-branch without NULL-scope disconnect */
    const char *or_and_ok =
        "// prompt: disjunction before conjunction preserves inner bounds.\n"
        "pub def main(flagOne: Bool, flagTwo: Bool, count: U64, items: Array[U64](count), index: U64): U64 <lessOrEqual(0, result)> =\n"
        "  if (flagOne || flagTwo) && index < count then items(index) else 0\n";
    p2s[0] = or_and_ok; p2l[0] = strlen(or_and_ok);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p2s, p2l, 1, &r, &d) == STIGI_OK, "check or_and_ok");
    if (!r.accepted) dump_rejects("or_and_ok", &r);
    CHECK(r.accepted, "Phase 2.1: (A || B) && C credits C cleanly");
    stigi_door_result_free(&r);

    /* (c) val Ctor(a, b) = expr destructuring carries array length & enum case facts */
    const char *pair_enum =
        "// prompt: single-case enum carrying slice and valid index.\n"
        "pub enum Demo.SliceView:\n"
        "  case View(count: U64, items: Array[U64](count), index: U64) <below(index, count)>\n";
    const char *val_ctor_ok =
        "// prompt: destructure single-case enum with val and index directly.\n"
        "pub def main(view: Demo.SliceView): U64 <lessOrEqual(0, result)> =\n"
        "  val View(count, items, index) = view\n"
        "  items(index)\n";
    p2s[0] = pair_enum; p2l[0] = strlen(pair_enum);
    p2s[1] = val_ctor_ok; p2l[1] = strlen(val_ctor_ok);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p2s, p2l, 2, &r, &d) == STIGI_OK, "check val_ctor_ok");
    if (!r.accepted) dump_rejects("val_ctor_ok", &r);
    CHECK(r.accepted, "Phase 2.1: val Ctor(...) destructuring carries array length & case facts");
    stigi_door_result_free(&r);

    /* (d) nominal refinement type T = U <facts> credits on entry and owes refinement-unproved */
    const char *ref_ty =
        "// prompt: refined index bounded below 10.\n"
        "pub type Demo.Small = U64 <below(self, 10)>\n";
    const char *ref_ok =
        "// prompt: refined parameter proves safe addition without explicit entry group.\n"
        "pub def main(small: Demo.Small): U64 = small + 1\n";
    const char *ref_bad =
        "// prompt: unguarded return into refined type must fail refinement-unproved.\n"
        "pub def main(raw: U64): Demo.Small = raw\n";
    p2s[0] = ref_ty; p2l[0] = strlen(ref_ty);
    p2s[1] = ref_ok; p2l[1] = strlen(ref_ok);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p2s, p2l, 2, &r, &d) == STIGI_OK, "check ref_ok");
    if (!r.accepted) dump_rejects("ref_ok", &r);
    CHECK(r.accepted, "Phase 2.1: nominal type T = U <facts> credits entry bound");
    stigi_door_result_free(&r);

    p2s[1] = ref_bad; p2l[1] = strlen(ref_bad);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p2s, p2l, 2, &r, &d) == STIGI_OK, "check ref_bad");
    CHECK(!r.accepted && has_reject_code(&r, "refinement-unproved"),
          "Phase 2.1: unguarded conversion to refined type rejected with refinement-unproved");
    stigi_door_result_free(&r);

    /* (e) protocol-only capability signature rejects concrete handler in {...} */
    const char *cap_u =
        "// prompt: counter capability protocol.\n"
        "capability Demo.Counter:\n"
        "  def next(): U64\n";
    const char *hdl_u =
        "// prompt: concrete handler for Demo.Counter.\n"
        "pub handler Demo.MemCounter(initial: U64) for Demo.Counter:\n"
        "  var current: U64 = initial\n"
        "  def next(): U64 = current\n";
    const char *bad_proto =
        "// prompt: signature naming concrete handler must fail concrete-protocol-in-signature.\n"
        "pub def main {counter: Demo.MemCounter} (): U64 <lessOrEqual(0, result)> = 0\n";
    p2s[0] = cap_u; p2l[0] = strlen(cap_u);
    p2s[1] = hdl_u; p2l[1] = strlen(hdl_u);
    p2s[2] = bad_proto; p2l[2] = strlen(bad_proto);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p2s, p2l, 3, &r, &d) == STIGI_OK, "check bad_proto");
    if (!has_reject_code(&r, "concrete-protocol-in-signature")) dump_rejects("bad_proto", &r);
    CHECK(!r.accepted && has_reject_code(&r, "concrete-protocol-in-signature"),
          "Phase 2.1: concrete handler in signature rejected with concrete-protocol-in-signature");
    stigi_door_result_free(&r);

    /* (f) Phase 2.2+2.3: nested case facts + if-guard bounds + val-else Array.tryFill + multi-head + pipe */
    const char *wrap_enum =
        "// prompt: outer wrapper around verified slice view.\n"
        "pub enum Demo.WrappedSlice:\n"
        "  case Wrapped(view: Demo.SliceView, bonus: U64)\n"
        "  case EmptySlice\n";
    const char *region_u =
        "// prompt: minimal region capability for val-else allocation.\n"
        "pub capability Stigi.Region:\n"
        "  def scratch(): U64\n";
    const char *add_u =
        "// prompt: add two words with overflow bound.\n"
        "pub def Demo.bump(value: U64, delta: U64) <below(value, 1000), below(delta, 1000)>: U64 =\n"
        "  value + delta\n";
    const char *p22_ok =
        "// prompt: nested pattern case facts, guard bounds, val-else alloc, multi-head, pipe.\n"
        "pub def main {region: Stigi.Region} (wrap: Demo.WrappedSlice, extra: U64) <below(extra, 100)>: U64 =\n"
        "  val base: U64 = wrap match\n"
        "    case Wrapped(View(count, items, index), bonus) if bonus < 100 =>\n"
        "      val elem: U64 = items(index)\n"
        "      if elem < 500 then elem |> Demo.bump(bonus) else bonus\n"
        "    case Wrapped(View(_, _, _), _) => 0\n"
        "    case EmptySlice => 0\n"
        "  val Filled(scratch) = Array.tryFill{region}(4, extra) else\n"
        "    case FillFailed => base\n"
        "  scratch(0)\n";
    const char *p22s[5] = {pair_enum, wrap_enum, region_u, add_u, p22_ok};
    size_t p22l[5];
    for (int i = 0; i < 5; i++) p22l[i] = strlen(p22s[i]);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, p22s, p22l, 5, &r, &d) == STIGI_OK, "check p22_ok");
    if (!r.accepted) dump_rejects("p22_ok", &r);
    CHECK(r.accepted, "Phase 2.2+2.3: nested case facts + guard bounds + val-else + pipe pass full door+tupu");
    stigi_door_result_free(&r);

    /* (g) Phase 2.4 (S125): trait/given + Bezem-bounded uKanren pos + all 5 neg codes */
    const char *eq_trait =
        "// prompt: structural equality trait protocol.\n"
        "pub trait Demo.Equal [A: Type]:\n"
        "  def equal(left: A, right: A): Bool\n";
    const char *eq_word =
        "// prompt: equality given for U64 words.\n"
        "pub given Demo.Equal.Word: Demo.Equal[U64]:\n"
        "  def equal(left: U64, right: U64): Bool = left == right\n";
    const char *maybe_enum =
        "// prompt: generic optional container.\n"
        "pub enum Demo.Maybe [A: Type]:\n"
        "  case Some(value: A)\n"
        "  case None\n";
    const char *eq_maybe =
        "// prompt: recursive equality given for Demo.Maybe.\n"
        "pub given Demo.Equal.Maybe [A: Type] {Demo.Equal[A]}: Demo.Equal[Demo.Maybe[A]]:\n"
        "  def equal(left: Demo.Maybe[A], right: Demo.Maybe[A]): Bool =\n"
        "    case (Some(first), Some(second)) => Demo.Equal.equal(first, second)\n"
        "    case (None, None) => true\n"
        "    case (Some(_), None) => false\n"
        "    case (None, Some(_)) => false\n";
    const char *same_def =
        "// prompt: generic helper requiring implicit Demo.Equal protocol.\n"
        "pub def Demo.same [A: Type] {Demo.Equal[A]} (first: A, second: A): Bool =\n"
        "  Demo.Equal.equal(first, second)\n";
    const char *eq_test_ok =
        "// prompt: verify uKanren concrete and recursive given resolution.\n"
        "val App.Tests.givenPass {Stigi.Test}: Stigi.Test.Result =\n"
        "  val direct: Bool = Demo.same[U64](7, 7)\n"
        "  val nested: Bool = Demo.Equal.equal[Demo.Maybe[U64]](Some(9), Some(9))\n"
        "  val diff: Bool = Demo.Equal.equal[Demo.Maybe[U64]](Some(9), None)\n"
        "  Stigi.Test.expect(direct && nested && !diff, true, \"uKanren resolves concrete and recursive givens\")\n";
    const char *kpos[6] = {eq_trait, eq_word, maybe_enum, eq_maybe, same_def, eq_test_ok};
    size_t kposl[6];
    for (int i = 0; i < 6; i++) kposl[i] = strlen(kpos[i]);
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kpos, kposl, 6, &r, &d) == STIGI_OK, "check kanren pos");
    if (!r.accepted) dump_rejects("kanren_pos", &r);
    CHECK(r.accepted, "Phase 2.4: trait/given + recursive uKanren + Test eval pass full door+tupu");
    stigi_door_result_free(&r);

    /* NEG 1: no-given */
    const char *neg_no_given =
        "// prompt: call Demo.Equal.equal on Bool with no given.\n"
        "pub def main(): Bool = Demo.Equal.equal(true, false)\n";
    const char *kn1[3] = {eq_trait, eq_word, neg_no_given};
    size_t kn1l[3] = {strlen(kn1[0]), strlen(kn1[1]), strlen(kn1[2])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn1, kn1l, 3, &r, &d) == STIGI_OK, "check no-given");
    if (!has_reject_code(&r, "no-given")) dump_rejects("no-given", &r);
    CHECK(!r.accepted && has_reject_code(&r, "no-given"), "Phase 2.4: missing given rejected with no-given");
    stigi_door_result_free(&r);

    /* NEG 2: ambiguous-givens */
    const char *eq_word_dup =
        "// prompt: competing equality given for U64 words.\n"
        "pub given Demo.Equal.WordAlt: Demo.Equal[U64]:\n"
        "  def equal(left: U64, right: U64): Bool = left == right\n";
    const char *use_word_eq =
        "// prompt: call Demo.Equal.equal on U64 with two competing givens.\n"
        "pub def main(): Bool = Demo.Equal.equal(3, 3)\n";
    const char *kn2[4] = {eq_trait, eq_word, eq_word_dup, use_word_eq};
    size_t kn2l[4] = {strlen(kn2[0]), strlen(kn2[1]), strlen(kn2[2]), strlen(kn2[3])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn2, kn2l, 4, &r, &d) == STIGI_OK, "check ambiguous-givens");
    if (!has_reject_code(&r, "ambiguous-givens")) dump_rejects("ambiguous-givens", &r);
    CHECK(!r.accepted && has_reject_code(&r, "ambiguous-givens"),
          "Phase 2.4: competing givens rejected with ambiguous-givens");
    stigi_door_result_free(&r);

    /* NEG 3: given-fuel (cyclic non-decreasing given) */
    const char *eq_loop =
        "// prompt: cyclic given that triggers Bezem fuel limit.\n"
        "pub given Demo.Equal.Cycle [A: Type] {Demo.Equal[A]}: Demo.Equal[A]:\n"
        "  def equal(left: A, right: A): Bool = Demo.Equal.equal(left, right)\n";
    const char *kn3[3] = {eq_trait, eq_loop, neg_no_given};
    size_t kn3l[3] = {strlen(kn3[0]), strlen(kn3[1]), strlen(kn3[2])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn3, kn3l, 3, &r, &d) == STIGI_OK, "check given-fuel");
    if (!has_reject_code(&r, "given-fuel")) dump_rejects("given-fuel", &r);
    CHECK(!r.accepted && has_reject_code(&r, "given-fuel"),
          "Phase 2.4: cyclic given rejected with given-fuel");
    stigi_door_result_free(&r);

    /* NEG 4: given-row (effectful capability in given) */
    const char *eq_eff =
        "// prompt: effectful given carrying Stigi.Mut must fail given-row.\n"
        "pub given Demo.Equal.Effectful {Stigi.Mut}: Demo.Equal[U64]:\n"
        "  def equal(left: U64, right: U64): Bool = left == right\n";
    const char *kn4[3] = {eq_trait, eq_eff, use_word_eq};
    size_t kn4l[3] = {strlen(kn4[0]), strlen(kn4[1]), strlen(kn4[2])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn4, kn4l, 3, &r, &d) == STIGI_OK, "check given-row");
    if (!has_reject_code(&r, "given-row")) dump_rejects("given-row", &r);
    CHECK(!r.accepted && has_reject_code(&r, "given-row"),
          "Phase 2.4: effectful given rejected with given-row");
    stigi_door_result_free(&r);

    /* NEG S137: a fact stated on a relation unit under Stigi.* is owed at its point and nothing clears it:
     * the assertion is rejected with assertion-unproved (the schema's atomOwed has no clearing rule) */
    const char *owed_relation =
        "// prompt: kernel relation with no rows for the owed-assertion probe.\n"
        "pub relation Stigi.Probe.rows(value: Stigi.Id.Value, limit: Stigi.Id.Value)\n";
    const char *owed_assert =
        "// prompt: assert a row of the kernel relation that no rule derives.\n"
        "pub def main(item: U64, limit: U64) <lessOrEqual(0, item)>: Bool =\n"
        "  @<Stigi.Probe.rows(item, limit)>\n"
        "  false\n";
    const char *kn_owed[2] = {owed_relation, owed_assert};
    size_t kn_owedl[2] = {strlen(kn_owed[0]), strlen(kn_owed[1])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn_owed, kn_owedl, 2, &r, &d) == STIGI_OK, "check owed relation assertion");
    if (!has_reject_code(&r, "assertion-unproved")) dump_rejects("owed-relation", &r);
    CHECK(!r.accepted && has_reject_code(&r, "assertion-unproved"),
          "S137: an assertion on a Stigi.* relation unit is rejected with assertion-unproved");
    stigi_door_result_free(&r);

    /* The same relation promised as an exit fact is owed too (exit-unproved), through the relation-unit producer */
    const char *owed_exit =
        "// prompt: promise a row of the kernel relation at exit that no rule derives.\n"
        "pub def main(item: U64, limit: U64) <lessOrEqual(0, item)>: Bool <Stigi.Probe.rows(item, limit)> =\n"
        "  false\n";
    const char *kn_exit[2] = {owed_relation, owed_exit};
    size_t kn_exitl[2] = {strlen(kn_exit[0]), strlen(kn_exit[1])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn_exit, kn_exitl, 2, &r, &d) == STIGI_OK, "check owed relation exit fact");
    if (!has_reject_code(&r, "exit-unproved")) dump_rejects("owed-exit", &r);
    CHECK(!r.accepted && has_reject_code(&r, "exit-unproved"),
          "S137: an exit fact on a Stigi.* relation unit is rejected with exit-unproved");
    stigi_door_result_free(&r);

    /* NEG 5: concrete-protocol-in-signature for given */
    const char *eq_trait_priv =
        "// prompt: private equality trait protocol.\n"
        "trait Demo.Equal [A: Type]:\n"
        "  def equal(left: A, right: A): Bool\n";
    const char *bad_given_sig =
        "// prompt: signature naming concrete given must fail concrete-protocol-in-signature.\n"
        "pub def main {wordEqual: Demo.Equal.Word} (): Bool = true\n";
    const char *kn5[3] = {eq_trait_priv, eq_word, bad_given_sig};
    size_t kn5l[3] = {strlen(kn5[0]), strlen(kn5[1]), strlen(kn5[2])};
    stigi_door_result_init(&r);
    CHECK(stigi_door_check(&p2door, kn5, kn5l, 3, &r, &d) == STIGI_OK, "check concrete given sig");
    if (!has_reject_code(&r, "concrete-protocol-in-signature")) dump_rejects("concrete_given_sig", &r);
    CHECK(!r.accepted && has_reject_code(&r, "concrete-protocol-in-signature"),
          "Phase 2.4: concrete given in signature rejected with concrete-protocol-in-signature");
    stigi_door_result_free(&r);

    stigi_door_free(&p2door);
  }

  /* 10. C7 export parity (S134 2): rules.dl once; one fact directory per door evaluation; tupu on the
   * directory of an admitted unit gives EXACTLY 0 reject rows; a NegTest body is a directory of its local
   * units' directories and tupu gives at least one reject row over them */
  {
    char base[] = "/tmp/stigi-c7-export-XXXXXX";
    CHECK(mkdtemp(base) != NULL, "mkdtemp export base");
    char root[sizeof base + 8];
    (void)snprintf(root, sizeof root, "%s/export", base);
    StigiExportStats xs;
    CHECK(stigi_door_export(&door, root, &xs, &d) == STIGI_OK, "stigi_door_export");
    const char *tupu_bin = getenv("TUPU_BIN");
    if (tupu_bin == NULL || tupu_bin[0] == '\0') {
      tupu_bin = "/home/vic/hk/tupu/result/bin/tupu";
    }
    DIR *dp = opendir(root);
    CHECK(dp != NULL, "open export directory");
    uint32_t unit_dirs = 0, negtest_dirs = 0, local_dirs = 0;
    int unit_rej = 0, negtest_rej = 0;
    struct dirent *de;
    while (dp != NULL && (de = readdir(dp)) != NULL) {
      if (de->d_name[0] == '.' || strcmp(de->d_name, "rules.dl") == 0) continue;
      char path[1024];
      (void)snprintf(path, sizeof path, "%s/%s", root, de->d_name);
      bool is_negtest_dir = false;
      for (uint32_t u = 0; u < door.image.unit_count; u++) {
        if (door.image.units[u].ast->decl->kind != STIGI_DECL_NEG_TEST) continue;
        char hex[65];
        for (int b = 0; b < 32; b++) (void)snprintf(hex + 2 * b, 3, "%02x", (unsigned)door.image.units[u].unit_id[b]);
        if (strncmp(de->d_name, hex, 64) == 0) is_negtest_dir = true;
      }
      if (!is_negtest_dir) {
        unit_dirs++;
        int rows = tupu_reject_rows(tupu_bin, root, path);
        CHECK(rows >= 0, "tupu on an admitted unit's fact directory prints the reject relation");
        if (rows > 0) unit_rej += rows;
        continue;
      }
      negtest_dirs++;
      int rows_here = 0;
      DIR *ndp = opendir(path);
      CHECK(ndp != NULL, "open NegTest export directory");
      struct dirent *le;
      while (ndp != NULL && (le = readdir(ndp)) != NULL) {
        if (le->d_name[0] == '.') continue;
        char lpath[2048];
        (void)snprintf(lpath, sizeof lpath, "%s/%s", path, le->d_name);
        local_dirs++;
        int rows = tupu_reject_rows(tupu_bin, root, lpath);
        CHECK(rows >= 0, "tupu on a NegTest local unit's fact directory prints the reject relation");
        if (rows > 0) rows_here += rows;
      }
      if (ndp != NULL) (void)closedir(ndp);
      CHECK(rows_here > 0, "a NegTest body shows at least one reject row over its local directories");
      negtest_rej += rows_here;
    }
    if (dp != NULL) (void)closedir(dp);
    CHECK(unit_dirs == xs.unit_dirs, "one fact directory per admitted unit");
    CHECK(negtest_dirs == xs.negtest_dirs, "one directory per NegTest body with Datalog");
    CHECK(local_dirs == xs.negtest_local_dirs, "one directory per NegTest local unit");
    CHECK(unit_dirs > 0, "export holds at least one evaluation");
    CHECK(unit_rej == 0, "Wave 3: tupu on every admitted unit's fact directory gives EXACTLY 0 reject rows");
    CHECK(negtest_rej >= (int)xs.negtest_dirs, "tupu on the NegTest bodies gives their reject rows");
  }

  {
    const char *tupu_bin = getenv("TUPU_BIN");
    if (tupu_bin == NULL || tupu_bin[0] == '\0') tupu_bin = "/home/vic/hk/tupu/result/bin/tupu";
    test_s138_rule_mapping(&door, tupu_bin);
  }
  test_s129_negatives(&door);

  stigi_door_free(&door);

  if (g_failures == 0) {
    printf("c5_c6_c7_test: ALL PASS\n");
    return 0;
  }
  fprintf(stderr, "c5_c6_c7_test: %d FAILURES\n", g_failures);
  return 1;
}
