#include "ast.h"
#include "common.h"
#include "eval.h"
#include "names.h"
#include "types.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                           \
  do {                                                                             \
    if (!(cond)) {                                                                 \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));              \
      g_failures++;                                                                \
    }                                                                              \
  } while (0)

static bool has_name_code(const StigiNameDiags *d, StigiNameCode code) {
  for (uint32_t i = 0; i < d->count; i++) {
    if (d->items[i].code == code) return true;
  }
  return false;
}

static bool has_type_code(const StigiTypeDiags *d, StigiTypeCode code) {
  for (uint32_t i = 0; i < d->count; i++) {
    if (d->items[i].code == code) return true;
  }
  return false;
}

static void add_unit(StigiImage *img, const char *src, StigiNameDiags *diags) {
  Diag d;
  diag_clear(&d);
  Status st = stigi_image_add_source(img, src, strlen(src), diags, &d);
  if (st != STIGI_OK) {
    fprintf(stderr, "FAIL add_unit: %s\n", d.msg);
    g_failures++;
  }
}

/* ---- C3 Tests ---- */

static void test_names_lookup_and_id_invariance(void) {
  StigiImage img1, img2;
  StigiNameDiags nd1, nd2;
  stigi_image_init(&img1);
  stigi_image_init(&img2);
  stigi_name_diags_init(&nd1);
  stigi_name_diags_init(&nd2);

  const char *v1 =
      "// prompt: return larger of two numbers.\n"
      "pub def Demo.larger(left: U64, right: U64): U64 =\n"
      "  if left < right then right else left\n";

  const char *v2 =
      "// prompt: DIFFERENT prompt text with extra comments and blank lines.\n"
      "\n"
      "// trailing comment\n"
      "pub def Demo.larger(left: U64, right: U64): U64 =\n"
      "  // inside body comment\n"
      "  if left < right then right else left\n"
      "\n";

  add_unit(&img1, v1, &nd1);
  add_unit(&img2, v2, &nd2);

  int32_t idx1 = stigi_image_find(&img1, "Demo.larger", strlen("Demo.larger"));
  int32_t idx2 = stigi_image_find(&img2, "Demo.larger", strlen("Demo.larger"));
  CHECK(idx1 == 0 && idx2 == 0, "O(1) lookup finds Demo.larger");
  CHECK(stigi_image_find(&img1, "Demo.missing", strlen("Demo.missing")) == -1,
        "O(1) lookup returns -1 for missing");
  CHECK(memcmp(img1.units[0].unit_id, img2.units[0].unit_id, 32) == 0,
        "BLAKE3 unit_id invariant across whitespace, comments, and prompt");

  stigi_name_diags_free(&nd1);
  stigi_name_diags_free(&nd2);
  stigi_image_free(&img1);
  stigi_image_free(&img2);
}

static void test_alpha_equiv_and_dep_propagation(void) {
  Diag d;
  diag_clear(&d);

  /* 1. Alpha-equivalence: different unit names and different parameter/local names => identical 32-byte BLAKE3 ID */
  {
    StigiImage img1, img2;
    StigiNameDiags nd1, nd2;
    stigi_image_init(&img1);
    stigi_image_init(&img2);
    stigi_name_diags_init(&nd1);
    stigi_name_diags_init(&nd2);

    add_unit(&img1,
             "// prompt: increment alpha.\n"
             "def Demo.foo(alpha: U64): U64 =\n"
             "  val temp: U64 = alpha + 1\n"
             "  temp\n",
             &nd1);
    add_unit(&img2,
             "// prompt: increment xyz.\n"
             "def Other.bar(xyz: U64): U64 =\n"
             "  val resultVal: U64 = xyz + 1\n"
             "  resultVal\n",
             &nd2);
    CHECK(stigi_image_resolve(&img1, &nd1, &d) == STIGI_OK, "resolve alpha 1");
    CHECK(stigi_image_resolve(&img2, &nd2, &d) == STIGI_OK, "resolve alpha 2");
    CHECK(memcmp(img1.units[0].unit_id, img2.units[0].unit_id, 32) == 0,
          "alpha-equivalent units produce identical 32-byte BLAKE3 unit_id");

    stigi_name_diags_free(&nd1);
    stigi_name_diags_free(&nd2);
    stigi_image_free(&img1);
    stigi_image_free(&img2);
  }

  /* 2. Dependency propagation: changing callee body changes caller 32-byte BLAKE3 ID even when caller text is untouched */
  {
    StigiImage img1, img2;
    StigiNameDiags nd1, nd2;
    stigi_image_init(&img1);
    stigi_image_init(&img2);
    stigi_name_diags_init(&nd1);
    stigi_name_diags_init(&nd2);

    const char *caller =
        "// prompt: calls helper.\n"
        "pub def Demo.caller(value: U64): U64 =\n"
        "  Demo.helper(value)\n";

    add_unit(&img1, "// prompt: add one.\ndef Demo.helper(value: U64): U64 = value + 1\n", &nd1);
    add_unit(&img1, caller, &nd1);

    add_unit(&img2, "// prompt: add two.\ndef Demo.helper(value: U64): U64 = value + 2\n", &nd2);
    add_unit(&img2, caller, &nd2);

    CHECK(stigi_image_resolve(&img1, &nd1, &d) == STIGI_OK, "resolve dep 1");
    CHECK(stigi_image_resolve(&img2, &nd2, &d) == STIGI_OK, "resolve dep 2");

    int32_t c1 = stigi_image_find(&img1, "Demo.caller", strlen("Demo.caller"));
    int32_t c2 = stigi_image_find(&img2, "Demo.caller", strlen("Demo.caller"));
    CHECK(c1 >= 0 && c2 >= 0, "find Demo.caller in both images");
    CHECK(memcmp(img1.units[c1].unit_id, img2.units[c2].unit_id, 32) != 0,
          "changing callee body changes caller 32-byte BLAKE3 unit_id");

    stigi_name_diags_free(&nd1);
    stigi_name_diags_free(&nd2);
    stigi_image_free(&img1);
    stigi_image_free(&img2);
  }

  /* 3. Ruling S119: runtime capability changes runtime_id; erased @<..> fact keeps runtime_id identical while changing verdict_key */
  {
    StigiImage img1, img2, img3;
    StigiNameDiags nd1, nd2, nd3;
    stigi_image_init(&img1);
    stigi_image_init(&img2);
    stigi_image_init(&img3);
    stigi_name_diags_init(&nd1);
    stigi_name_diags_init(&nd2);
    stigi_name_diags_init(&nd3);

    const char *cap_def =
        "// prompt: runtime clock capability.\n"
        "pub capability Demo.Clock:\n"
        "  def now(): U64\n";

    add_unit(&img1, cap_def, &nd1);
    add_unit(&img1,
             "// prompt: plain function.\n"
             "pub def Demo.step(value: U64): U64 =\n"
             "  value + 1\n",
             &nd1);

    add_unit(&img2, cap_def, &nd2);
    add_unit(&img2,
             "// prompt: function with runtime capability.\n"
             "pub def Demo.step {clock: Demo.Clock} (value: U64): U64 =\n"
             "  value + 1\n",
             &nd2);

    add_unit(&img3, cap_def, &nd3);
    add_unit(&img3,
             "// prompt: function with erased proof fact.\n"
             "pub def Demo.step(value: U64): U64 <lessOrEqual(value, result)> =\n"
             "  @<lessOrEqual(0, value)>\n"
             "  value + 1\n",
             &nd3);

    CHECK(stigi_image_resolve(&img1, &nd1, &d) == STIGI_OK, "resolve s119 1");
    CHECK(stigi_image_resolve(&img2, &nd2, &d) == STIGI_OK, "resolve s119 2");
    CHECK(stigi_image_resolve(&img3, &nd3, &d) == STIGI_OK, "resolve s119 3");

    int32_t s1 = stigi_image_find(&img1, "Demo.step", strlen("Demo.step"));
    int32_t s2 = stigi_image_find(&img2, "Demo.step", strlen("Demo.step"));
    int32_t s3 = stigi_image_find(&img3, "Demo.step", strlen("Demo.step"));
    CHECK(s1 >= 0 && s2 >= 0 && s3 >= 0, "find Demo.step in S119 images");

    /* (1) Adding a runtime capability changes runtime_id */
    CHECK(memcmp(img1.units[s1].unit_id, img2.units[s2].unit_id, 32) != 0,
          "S119: adding runtime capability changes runtime_id");

    /* (2) Adding erased @<..> and <..> facts keeps runtime_id identical while changing verdict_key */
    CHECK(memcmp(img1.units[s1].unit_id, img3.units[s3].unit_id, 32) == 0,
          "S119: adding erased @<..> fact keeps runtime_id identical");
    CHECK(memcmp(img1.units[s1].verdict_key, img3.units[s3].verdict_key, 32) != 0,
          "S119: adding erased @<..> fact changes verdict_key");

    stigi_name_diags_free(&nd1);
    stigi_name_diags_free(&nd2);
    stigi_name_diags_free(&nd3);
    stigi_image_free(&img1);
    stigi_image_free(&img2);
    stigi_image_free(&img3);
  }
}

static void test_names_diagnostics(void) {
  Diag d;
  diag_clear(&d);

  /* unresolved-name */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    add_unit(&img,
             "// prompt: calls nonexistent helper.\n"
             "pub def main(): I32 =\n"
             "  Demo.nonexistent(1)\n",
             &nd);
    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "resolve unresolved");
    CHECK(has_name_code(&nd, STIGI_NAME_UNRESOLVED), "unresolved-name reported");
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* private-reference */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    add_unit(&img,
             "// prompt: private helper.\n"
             "def Alpha.secret(value: U64): U64 =\n"
             "  value + 1\n",
             &nd);
    add_unit(&img,
             "// prompt: caller outside namespace.\n"
             "pub def main(): I32 =\n"
             "  Alpha.secret(1)\n",
             &nd);
    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "resolve private");
    CHECK(has_name_code(&nd, STIGI_NAME_PRIVATE_REF), "private-reference reported");
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* duplicate */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    add_unit(&img, "// prompt: first.\nval Demo.item: U64 = 1\n", &nd);
    add_unit(&img, "// prompt: second.\nval Demo.item: U64 = 2\n", &nd);
    CHECK(has_name_code(&nd, STIGI_NAME_DUPLICATE), "duplicate unit reported");
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* no-reader */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    add_unit(&img, "// prompt: dead helper.\ndef Demo.unused(value: U64): U64 = value\n", &nd);
    add_unit(&img, "// prompt: main.\npub def main(): I32 = 0\n", &nd);
    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "resolve no-reader");
    CHECK(has_name_code(&nd, STIGI_NAME_NO_READER), "no-reader reported");
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* pub-on-test */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    add_unit(&img,
             "// prompt: pub test.\n"
             "pub val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
             "  Stigi.Test.expect(1, 1, \"one equals one\")\n",
             &nd);
    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "resolve pub-on-test");
    CHECK(has_name_code(&nd, STIGI_NAME_PUB_ON_TEST), "pub-on-test reported");
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }
}

/* ---- C4 Type Checker & Linear Normalizer Tests ---- */

static void test_linear_normalizer(void) {
  StigiLinear n = stigi_linear_var(1);
  StigiLinear m = stigi_linear_var(2);
  StigiLinear two = stigi_linear_const(2);

  StigiLinear n_plus_2, two_plus_n;
  CHECK(stigi_linear_add(&n, &two, &n_plus_2) == STIGI_OK, "n + 2");
  CHECK(stigi_linear_add(&two, &n, &two_plus_n) == STIGI_OK, "2 + n");
  CHECK(stigi_linear_equal(&n_plus_2, &two_plus_n), "n + 2 == 2 + n");

  int64_t delta = 0;
  CHECK(stigi_linear_offset(&n_plus_2, &n, &delta) && delta == 2, "termOffset(n+2, n) == 2");

  StigiLinear prod;
  CHECK(stigi_linear_mul(&n, &m, &prod) != STIGI_OK, "n * m rejected as nonlinear");
}

static void test_typecheck_cases(void) {
  Diag d;
  diag_clear(&d);

  /* mismatch & assign-val & length-not-linear */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img,
             "// prompt: return bool from U64 def.\n"
             "def Demo.badRet(value: U64): U64 =\n"
             "  true\n",
             &nd);
    add_unit(&img,
             "// prompt: assign to val.\n"
             "def Demo.badAssign(value: U64): U64 =\n"
             "  val frozen: U64 = value\n"
             "  frozen = 2\n"
             "  frozen\n",
             &nd);
    add_unit(&img,
             "// prompt: nonlinear length.\n"
             "def Demo.badLen(rows: U64, cols: U64, buf: Array[U8](rows * cols)): U64 =\n"
             "  rows\n",
             &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck mismatch/assign/len");
    CHECK(has_type_code(&td, STIGI_TYPE_MISMATCH), "mismatch detected");
    CHECK(has_type_code(&td, STIGI_TYPE_ASSIGN_VAL), "assign-val detected");
    CHECK(has_type_code(&td, STIGI_TYPE_LENGTH_NOT_LINEAR), "length-not-linear detected");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* recursive-type: direct, through Array, through Cell, and accepted Tree with U64 index */
  {
    const char *bad_cases[3] = {
      "// prompt: direct recursion.\nenum Demo.Direct:\n  case Wrap(next: Demo.Direct)\n",
      "// prompt: array recursion.\nenum Demo.ViaArray:\n  case Wrap(items: Array[Demo.ViaArray](4))\n",
      "// prompt: cell recursion.\nenum Demo.ViaCell:\n  case Wrap(slot: Cell[Demo.ViaCell])\n"
    };
    for (int i = 0; i < 3; i++) {
      StigiImage img;
      StigiNameDiags nd;
      StigiTypeDiags td;
      stigi_image_init(&img);
      stigi_name_diags_init(&nd);
      stigi_type_diags_init(&td);
      add_unit(&img, bad_cases[i], &nd);
      CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck recursive");
      CHECK(has_type_code(&td, STIGI_TYPE_RECURSIVE_TYPE), "recursive-type detected");
      stigi_type_diags_free(&td);
      stigi_name_diags_free(&nd);
      stigi_image_free(&img);
    }

    /* Accepted Tree with U64 index */
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img,
             "// prompt: index-based tree node.\n"
             "enum Demo.TreeNode:\n"
             "  case Branch(left: U64, right: U64, value: U64)\n"
             "  case Leaf(value: U64)\n",
             &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck tree");
    CHECK(td.count == 0, "Tree with U64 index has 0 type errors");
    StigiTy ety;
    memset(&ety, 0, sizeof ety);
    ety.kind = STIGI_TY_ENUM;
    ety.unit_idx = 0;
    StigiLayout lay = stigi_ty_layout(&img, &ety);
    CHECK(lay.size == 32 && lay.align == 8, "TreeNode layout: tag(4)+pad(4)+3*U64(24)=32, align=8");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* extern-forges-facts: on return AND on overwritten buffer (S110) */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img, "// prompt: refined digit.\ntype Demo.Digit = U64 <below(self, 10)>\n", &nd);
    add_unit(&img, "// prompt: bad extern return.\nextern \"c\" def forgeDigit(): Demo.Digit\n", &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck extern ret");
    CHECK(has_type_code(&td, STIGI_TYPE_EXTERN_FORGES_FACTS), "extern-forges-facts on return");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img, "// prompt: refined digit.\ntype Demo.Digit = U64 <below(self, 10)>\n", &nd);
    add_unit(&img,
             "// prompt: bad extern overwrite.\n"
             "extern \"c\" def fillDigits(count: U64, items: Array[Demo.Digit](count)) <overwrites(items)>: Unit\n",
             &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck extern overwrite");
    CHECK(has_type_code(&td, STIGI_TYPE_EXTERN_FORGES_FACTS), "extern-forges-facts on overwrite");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }
}

/* ---- C4 Evaluator Tests ---- */

static void test_evaluator(void) {
  StigiImage img;
  StigiNameDiags nd;
  stigi_image_init(&img);
  stigi_name_diags_init(&nd);

  add_unit(&img,
           "// prompt: sum from 1 to limit.\n"
           "def Demo.sumTo(limit: U64): U64 =\n"
           "  var total: U64 = 0\n"
           "  var index: U64 = 0\n"
           "  while index < limit do\n"
           "    index = index + 1\n"
           "    total = total + index\n"
           "  total\n",
           &nd);
  add_unit(&img,
           "// prompt: positive test.\n"
           "val Demo.Tests.sumPass {Stigi.Test}: Stigi.Test.Result =\n"
           "  Stigi.Test.expect(Demo.sumTo(10), 55, \"sum 1..10 is 55\")\n",
           &nd);
  add_unit(&img,
           "// prompt: failing test.\n"
           "val Demo.Tests.sumFail {Stigi.Test}: Stigi.Test.Result =\n"
           "  Stigi.Test.expect(Demo.sumTo(10), 99, \"wrong expected sum\")\n",
           &nd);
  add_unit(&img,
           "// prompt: infinite loop.\n"
           "def Demo.spin(): U64 =\n"
           "  var count: U64 = 0\n"
           "  while true do\n"
           "    count = count + 1\n"
           "  count\n",
           &nd);
  add_unit(&img,
           "// prompt: extern fn.\n"
           "extern \"c\" def cRand(): U64\n",
           &nd);
  add_unit(&img,
           "// prompt: calls extern.\n"
           "val Demo.badVal: U64 = cRand()\n",
           &nd);

  Arena eval_arena;
  arena_init(&eval_arena);

  /* Positive Test passes */
  int32_t pass_idx = stigi_image_find(&img, "Demo.Tests.sumPass", strlen("Demo.Tests.sumPass"));
  CHECK(pass_idx >= 0, "find sumPass");
  StigiEvalResult r1 = stigi_eval_test(&img, (uint32_t)pass_idx, STIGI_DEFAULT_FUEL, &eval_arena);
  CHECK(r1.status == STIGI_OK && r1.value.kind == STIGI_VAL_TEST_OK, "positive Test passes");

  /* Failing Test gives test-failed */
  int32_t fail_idx = stigi_image_find(&img, "Demo.Tests.sumFail", strlen("Demo.Tests.sumFail"));
  CHECK(fail_idx >= 0, "find sumFail");
  StigiEvalResult r2 = stigi_eval_test(&img, (uint32_t)fail_idx, STIGI_DEFAULT_FUEL, &eval_arena);
  CHECK(r2.status != STIGI_OK && r2.diag_code != NULL && strcmp(r2.diag_code, "test-failed") == 0,
        "failing Test gives test-failed");

  /* Infinite loop hits evaluation-fuel */
  int32_t spin_idx = stigi_image_find(&img, "Demo.spin", strlen("Demo.spin"));
  CHECK(spin_idx >= 0, "find spin");
  StigiEvalResult r3 = stigi_eval_unit(&img, (uint32_t)spin_idx, 10000ULL, &eval_arena);
  CHECK(r3.status != STIGI_OK && r3.diag_code != NULL &&
            strcmp(r3.diag_code, "evaluation-fuel") == 0,
        "infinite loop hits evaluation-fuel");

  /* Body-less extern hits not-compile-time */
  int32_t ext_idx = stigi_image_find(&img, "Demo.badVal", strlen("Demo.badVal"));
  CHECK(ext_idx >= 0, "find badVal");
  StigiEvalResult r4 = stigi_eval_unit(&img, (uint32_t)ext_idx, STIGI_DEFAULT_FUEL, &eval_arena);
  CHECK(r4.status != STIGI_OK && r4.diag_code != NULL &&
            strcmp(r4.diag_code, "not-compile-time") == 0,
        "body-less extern hits not-compile-time");

  /* frameBytes check */
  int32_t sum_idx = stigi_image_find(&img, "Demo.sumTo", strlen("Demo.sumTo"));
  uint64_t fbytes = 0;
  CHECK(stigi_def_frame_bytes(&img, (uint32_t)sum_idx, &fbytes) == STIGI_OK && fbytes == 24,
        "sumTo frameBytes == 24 (limit + total + index)");

  /* Fix #12: unsigned U64 comparison & arithmetic, plus cyclic val depth guard */
  add_unit(&img,
           "// prompt: unsigned high-bit comparison.\n"
           "val Demo.Tests.u64Cmp {Stigi.Test}: Stigi.Test.Result =\n"
           "  Stigi.Test.expect(0x9e3779b97f4a7c15 < 1, false, \"unsigned high bit is > 1\")\n",
           &nd);
  add_unit(&img,
           "// prompt: unsigned INT64_MAX + 1.\n"
           "val Demo.Tests.u64Add {Stigi.Test}: Stigi.Test.Result =\n"
           "  Stigi.Test.expect(9223372036854775807 + 1, 9223372036854775808, \"INT64_MAX + 1 in U64\")\n",
           &nd);
  add_unit(&img,
           "// prompt: cyclic val.\n"
           "val Demo.cyclicVal: U64 = Demo.cyclicVal\n",
           &nd);

  int32_t ucmp = stigi_image_find(&img, "Demo.Tests.u64Cmp", strlen("Demo.Tests.u64Cmp"));
  StigiEvalResult r5 = stigi_eval_test(&img, (uint32_t)ucmp, STIGI_DEFAULT_FUEL, &eval_arena);
  CHECK(r5.status == STIGI_OK && r5.value.kind == STIGI_VAL_TEST_OK,
        "Fix #12: 0x9e3779b97f4a7c15 < 1 is false in U64");

  int32_t uadd = stigi_image_find(&img, "Demo.Tests.u64Add", strlen("Demo.Tests.u64Add"));
  StigiEvalResult r6 = stigi_eval_test(&img, (uint32_t)uadd, STIGI_DEFAULT_FUEL, &eval_arena);
  CHECK(r6.status == STIGI_OK && r6.value.kind == STIGI_VAL_TEST_OK,
        "Fix #12: INT64_MAX + 1 succeeds in U64");

  int32_t cyc = stigi_image_find(&img, "Demo.cyclicVal", strlen("Demo.cyclicVal"));
  StigiEvalResult r7 = stigi_eval_unit(&img, (uint32_t)cyc, STIGI_DEFAULT_FUEL, &eval_arena);
  CHECK(r7.status != STIGI_OK && r7.diag_code != NULL &&
            strcmp(r7.diag_code, "evaluation-fuel") == 0,
        "Fix #12: cyclic val returns evaluation-fuel instead of stack overflow");

  arena_free(&eval_arena);
  stigi_name_diags_free(&nd);
  stigi_image_free(&img);
}

static void test_pro_review_regressions(void) {
  Diag d;
  diag_clear(&d);

  /* Fixes #1 & #2: marker cap alias & lemma call erasure preserve runtime_id */
  {
    StigiImage img1, img2;
    StigiNameDiags nd1, nd2;
    stigi_image_init(&img1);
    stigi_image_init(&img2);
    stigi_name_diags_init(&nd1);
    stigi_name_diags_init(&nd2);

    const char *lem =
        "// prompt: trivial lemma.\n"
        "lemma def Demo.triv(value: U64) <lessOrEqual(0, value)>: <lessOrEqual(0, value)> =\n"
        "  @<lessOrEqual(0, value)>\n";
    add_unit(&img1, lem, &nd1);
    add_unit(&img1,
             "// prompt: plain step.\n"
             "pub def Demo.step(value: U64): U64 =\n"
             "  value + 1\n",
             &nd1);

    add_unit(&img2, lem, &nd2);
    add_unit(&img2,
             "// prompt: step with aliased marker cap and lemma call.\n"
             "alias MutCap = Stigi.Mut\n"
             "pub def Demo.step {MutCap} (value: U64): U64 =\n"
             "  Demo.triv(value)\n"
             "  value + 1\n",
             &nd2);

    CHECK(stigi_image_resolve(&img1, &nd1, &d) == STIGI_OK, "resolve s119 alias/lemma 1");
    CHECK(stigi_image_resolve(&img2, &nd2, &d) == STIGI_OK, "resolve s119 alias/lemma 2");
    int32_t s1 = stigi_image_find(&img1, "Demo.step", strlen("Demo.step"));
    int32_t s2 = stigi_image_find(&img2, "Demo.step", strlen("Demo.step"));
    CHECK(memcmp(img1.units[s1].unit_id, img2.units[s2].unit_id, 32) == 0,
          "Fixes #1/#2: aliased marker cap + lemma call erased in runtime_id");
    stigi_name_diags_free(&nd1);
    stigi_name_diags_free(&nd2);
    stigi_image_free(&img1);
    stigi_image_free(&img2);
  }

  /* Fix #3: ambiguous-group on symmetric mutual recursion */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    add_unit(&img, "// prompt: ping.\npub def Demo.ping(value: U64): U64 = Demo.pong(value)\n", &nd);
    add_unit(&img, "// prompt: pong.\npub def Demo.pong(value: U64): U64 = Demo.ping(value)\n", &nd);
    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "resolve ambiguous-group");
    CHECK(has_name_code(&nd, STIGI_NAME_AMBIGUOUS_GROUP), "Fix #3: ambiguous-group detected");
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* Fix #7: StigiLinear overflow checks */
  {
    StigiLinear maxv = stigi_linear_const(INT64_MAX);
    StigiLinear one = stigi_linear_const(1);
    StigiLinear out;
    CHECK(stigi_linear_add(&maxv, &one, &out) == STIGI_ERR_OVERFLOW,
          "Fix #7: StigiLinear add overflow detected");
    StigiLinear minv = stigi_linear_const(INT64_MIN);
    StigiLinear neg1 = stigi_linear_const(-1);
    CHECK(stigi_linear_mul(&minv, &neg1, &out) == STIGI_ERR_OVERFLOW,
          "Fix #7: StigiLinear mul INT64_MIN * -1 overflow detected");
  }

  /* Fixes #6, #9, #10, #11: overflow literal, extern-forges-facts (Option[Digit], Buf alias, extern val),
   * generic type arg inequality, var in length term, and non-local assign-val */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);

    add_unit(&img, "// prompt: digit.\ntype Demo.Digit = U64 <below(self, 10)>\n", &nd);
    add_unit(&img, "// prompt: option.\nenum Demo.Option [A: Type]:\n  case Some(value: A)\n  case None\n", &nd);
    add_unit(&img, "// prompt: color.\nenum Demo.Color:\n  case Red\n  case Blue\n", &nd);
    add_unit(&img, "// prompt: transparent buf.\ntype Demo.ColorBuf (size: U64) = Array[Demo.Color](size)\n", &nd);
    add_unit(&img, "// prompt: extern Option[Digit].\nextern \"c\" def makeOpt(): Demo.Option[Demo.Digit]\n", &nd);
    add_unit(&img, "// prompt: extern overwrites alias.\nextern \"c\" def fillBuf(size: U64, buf: Demo.ColorBuf(size)) <overwrites(buf)>: Unit\n", &nd);
    add_unit(&img, "// prompt: extern val Digit.\nextern \"c\" val rawDigit: Demo.Digit\n", &nd);
    add_unit(&img, "// prompt: overflow literal.\nval Demo.huge: U64 = 99999999999999999999999\n", &nd);
    add_unit(&img,
             "// prompt: assign to top-level val.\n"
             "def Demo.badTopAssign(): U64 =\n"
             "  Demo.huge = 1\n"
             "  1\n",
             &nd);
    add_unit(&img,
             "// prompt: var in length term.\n"
             "def Demo.varLen(items: Array[U8](1)): U64 =\n"
             "  var count: U64 = 1\n"
             "  val copy: Array[U8](count) = items\n"
             "  count\n",
             &nd);
    add_unit(&img,
             "// prompt: generic enum type arg mismatch.\n"
             "def Demo.optMismatch(item: Demo.Option[U8]): Demo.Option[U64] =\n"
             "  item\n",
             &nd);

    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck pro review regressions");
    uint32_t forge_cnt = 0;
    for (uint32_t i = 0; i < td.count; i++) {
      if (td.items[i].code == STIGI_TYPE_EXTERN_FORGES_FACTS) forge_cnt++;
    }
    CHECK(forge_cnt == 3, "Fix #9: all 3 extern-forges-facts cases detected");
    CHECK(has_type_code(&td, STIGI_TYPE_LITERAL_OUT_OF_RANGE), "Fix #6: u64 overflow literal rejected");
    CHECK(has_type_code(&td, STIGI_TYPE_ASSIGN_VAL), "Fix #11: non-local assign-val rejected");
    CHECK(has_type_code(&td, STIGI_TYPE_LENGTH_NOT_LINEAR), "Fix #10: var in length term rejected");
    CHECK(has_type_code(&td, STIGI_TYPE_MISMATCH), "Fix #10: Option[U8] != Option[U64] mismatch");

    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }
}

static void test_wave2_hardening(void) {
  Diag d;
  diag_clear(&d);

  /* 1. Strict canonical decoder: all 9 error reasons (identity.md:42-44) */
  {
    /* Truncated */
    uint8_t empty[1] = {0};
    CHECK(stigi_canon_decode(empty, 0).reason == STIGI_DECODE_TRUNCATED,
          "decode: empty -> Truncated");

    /* WrongSchema */
    uint8_t bad_schema[] = { 4, 'b', 'a', 'd', '!' };
    CHECK(stigi_canon_decode(bad_schema, sizeof bad_schema).reason == STIGI_DECODE_WRONG_SCHEMA,
          "decode: bad schema -> WrongSchema");

    /* NonMinimalNumber: overlong ULEB128 0x80 0x00 for string length */
    uint8_t nonmin[] = { 0x80, 0x00 };
    CHECK(stigi_canon_decode(nonmin, sizeof nonmin).reason == STIGI_DECODE_NON_MINIMAL_NUMBER,
          "decode: overlong ULEB -> NonMinimalNumber");

    /* NumberTooLarge: 10-byte ULEB with 10th byte 0x02 (> 2^64 - 1) */
    uint8_t toolarge[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02 };
    CHECK(stigi_canon_decode(toolarge, sizeof toolarge).reason == STIGI_DECODE_NUMBER_TOO_LARGE,
          "decode: >2^64-1 ULEB -> NumberTooLarge");

    /* UnknownTag: valid stigi/unit/v1 header + bogus decl tag 0xFF */
    uint8_t unk_tag[] = {
      13, 's','t','i','g','i','/','u','n','i','t','/','v','1',
      0xFF
    };
    CHECK(stigi_canon_decode(unk_tag, sizeof unk_tag).reason == STIGI_DECODE_UNKNOWN_TAG,
          "decode: unknown decl tag -> UnknownTag");

    /* UnboundLocal: stigi/unit/v1 DEF (0 tparams, 0 params, 0 rtcaps, ret U64, body LOCAL(5)) */
    uint8_t unbound[] = {
      13, 's','t','i','g','i','/','u','n','i','t','/','v','1',
      (uint8_t)(0x80 + STIGI_DECL_DEF),
      0, 0, 0, /* 0 tparams, 0 params, 0 rtcaps */
      (uint8_t)(0x10 + STIGI_TYPE_NAMED), 0x07, 3, 'U','6','4', 0, 0, 0, /* ret_type U64 */
      (uint8_t)(0x30 + STIGI_EXPR_LOCAL),
      0x01, 5 /* Local(5) with 0 binders in scope! */
    };
    CHECK(stigi_canon_decode(unbound, sizeof unbound).reason == STIGI_DECODE_UNBOUND_LOCAL,
          "decode: Local(5) out of scope -> UnboundLocal");

    /* MemberOutOfRange: stigi/unit/v1 DEF referencing SelfGroup(0) when group_size == 0 */
    uint8_t mem_oob[] = {
      13, 's','t','i','g','i','/','u','n','i','t','/','v','1',
      (uint8_t)(0x80 + STIGI_DECL_DEF),
      0, 0, 0,
      (uint8_t)(0x10 + STIGI_TYPE_NAMED), 0x07, 3, 'U','6','4', 0, 0, 0,
      (uint8_t)(0x30 + STIGI_EXPR_GLOBAL),
      0x05, 0 /* SelfGroup(0) in non-group unit */
    };
    CHECK(stigi_canon_decode(mem_oob, sizeof mem_oob).reason == STIGI_DECODE_MEMBER_OUT_OF_RANGE,
          "decode: SelfGroup in non-group -> MemberOutOfRange");

    /* TrailingBytes: valid stigi/unit/v1 DEF + 1 extra byte */
    uint8_t trailing[] = {
      13, 's','t','i','g','i','/','u','n','i','t','/','v','1',
      (uint8_t)(0x80 + STIGI_DECL_DEF),
      0, 0, 0,
      (uint8_t)(0x10 + STIGI_TYPE_NAMED), 0x07, 3, 'U','6','4', 0, 0, 0,
      (uint8_t)(0x30 + STIGI_EXPR_LIT_INT), 42,
      0x99 /* trailing byte */
    };
    CHECK(stigi_canon_decode(trailing, sizeof trailing - 1).reason == STIGI_DECODE_OK,
          "decode: valid stigi/unit/v1 decodes OK");
    CHECK(stigi_canon_decode(trailing, sizeof trailing).reason == STIGI_DECODE_TRAILING_BYTES,
          "decode: trailing byte -> TrailingBytes");

    /* NonCanonicalGroupOrder: stigi/group/v1 with 2 members in reverse order (lit 2 before lit 1) */
    uint8_t m2[] = {
      13, 's','t','i','g','i','/','u','n','i','t','/','v','1',
      (uint8_t)(0x80 + STIGI_DECL_DEF), 0, 0, 0,
      (uint8_t)(0x10 + STIGI_TYPE_NAMED), 0x07, 3, 'U','6','4', 0, 0, 0,
      (uint8_t)(0x30 + STIGI_EXPR_LIT_INT), 2
    };
    uint8_t m1[] = {
      13, 's','t','i','g','i','/','u','n','i','t','/','v','1',
      (uint8_t)(0x80 + STIGI_DECL_DEF), 0, 0, 0,
      (uint8_t)(0x10 + STIGI_TYPE_NAMED), 0x07, 3, 'U','6','4', 0, 0, 0,
      (uint8_t)(0x30 + STIGI_EXPR_LIT_INT), 1
    };
    uint8_t bad_grp[128];
    size_t gp = 0;
    bad_grp[gp++] = 14; memcpy(bad_grp + gp, "stigi/group/v1", 14); gp += 14;
    bad_grp[gp++] = 2; /* count = 2 */
    bad_grp[gp++] = (uint8_t)sizeof m2; memcpy(bad_grp + gp, m2, sizeof m2); gp += sizeof m2;
    bad_grp[gp++] = (uint8_t)sizeof m1; memcpy(bad_grp + gp, m1, sizeof m1); gp += sizeof m1;
    CHECK(stigi_canon_decode(bad_grp, gp).reason == STIGI_DECODE_NON_CANONICAL_GROUP_ORDER,
          "decode: unsorted SCC members -> NonCanonicalGroupOrder");
  }

  /* 2. S119: capability-op binder rename + comptime-only local val erasure + n+1 == 1+n */
  {
    StigiImage img1, img2;
    StigiNameDiags nd1, nd2;
    stigi_image_init(&img1);
    stigi_image_init(&img2);
    stigi_name_diags_init(&nd1);
    stigi_name_diags_init(&nd2);

    const char *cap =
        "// prompt: runtime clock.\n"
        "pub capability Demo.Clock:\n"
        "  def now(): U64\n";
    add_unit(&img1, cap, &nd1);
    add_unit(&img1,
             "// prompt: read clock with binder alpha.\n"
             "pub def Demo.readOne {clock: Demo.Clock} (count: U64, items: Array[U8](count + 1)): U64 =\n"
             "  clock.now()\n",
             &nd1);

    add_unit(&img2, cap, &nd2);
    add_unit(&img2,
             "// prompt: read clock with renamed binder + unused comptime val + 1 + count.\n"
             "pub def Demo.readTwo {timer: Demo.Clock} (size: U64, buf: Array[U8](1 + size)): U64 =\n"
             "  val witness: U64 = size + 1\n"
             "  @<lessOrEqual(size, witness)>\n"
             "  timer.now()\n",
             &nd2);

    CHECK(stigi_image_resolve(&img1, &nd1, &d) == STIGI_OK &&
          !has_name_code(&nd1, STIGI_NAME_UNRESOLVED),
          "resolve cap-op 1 without unresolved-name");
    CHECK(stigi_image_resolve(&img2, &nd2, &d) == STIGI_OK &&
          !has_name_code(&nd2, STIGI_NAME_UNRESOLVED),
          "resolve cap-op 2 without unresolved-name");
    int32_t u1 = stigi_image_find(&img1, "Demo.readOne", strlen("Demo.readOne"));
    int32_t u2 = stigi_image_find(&img2, "Demo.readTwo", strlen("Demo.readTwo"));
    CHECK(u1 >= 0 && u2 >= 0, "find readOne/readTwo");
    CHECK(memcmp(img1.units[u1].unit_id, img2.units[u2].unit_id, 32) == 0,
          "S119: cap-op binder rename + comptime-only local val erasure + n+1==1+n keep runtime_id equal");
    CHECK(memcmp(img1.units[u1].verdict_key, img2.units[u2].verdict_key, 32) != 0,
          "S119: extra @<..> fact still changes verdict_key");

    stigi_name_diags_free(&nd1);
    stigi_name_diags_free(&nd2);
    stigi_image_free(&img1);
    stigi_image_free(&img2);
  }

  /* 3. Never is bottom ONLY: `def bad(): Never = 42` MUST fail with mismatch */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img, "// prompt: invalid Never return.\npub def Demo.badNever(): Never = 42\n", &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck badNever");
    CHECK(has_type_code(&td, STIGI_TYPE_MISMATCH), "Never cannot accept U64");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* 4. Generic [A: Type] inference, cannot-infer, out-of-order dependent params, and qualified Enum.Case */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img,
             "// prompt: generic option.\n"
             "pub enum Demo.Box[A: Type]:\n"
             "  case Full(item: A)\n"
             "  case Empty\n",
             &nd);
    add_unit(&img,
             "// prompt: generic identity.\n"
             "pub def Demo.ident[A: Type](value: A): A = value\n",
             &nd);
    add_unit(&img,
             "// prompt: out-of-order dependent params.\n"
             "pub def Demo.useSlice(buf: Array[U8](count), count: U64): U64 = count\n",
             &nd);
    add_unit(&img,
             "// prompt: caller using inferred generics, dependent call len, and qualified enum ctor.\n"
             "pub def Demo.caller(len: U64, bytes: Array[U8](len)): Demo.Box[U64] =\n"
             "  val same: U64 = Demo.ident(Demo.useSlice(bytes, len))\n"
             "  Demo.Box.Full(same)\n",
             &nd);
    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "resolve generics+dep");
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK && td.count == 0,
          "generics + out-of-order dependent params + qualified Enum.Case pass with 0 type errors");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* 5. Dependent call length mismatch caught (Array[U8](len) passed to Array[U8](len + 1)) */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img,
             "// prompt: expects len + 1.\n"
             "pub def Demo.needPlus(count: U64, buf: Array[U8](count + 1)): U64 = count\n",
             &nd);
    add_unit(&img,
             "// prompt: passes len instead of len + 1.\n"
             "pub def Demo.badCall(size: U64, buf: Array[U8](size)): U64 =\n"
             "  Demo.needPlus(size, buf)\n",
             &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck dep len mismatch");
    CHECK(has_type_code(&td, STIGI_TYPE_MISMATCH), "dependent call length mismatch caught");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* 6. Handler signature mismatch caught (capabilities.md 2) */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);
    add_unit(&img,
             "// prompt: clock cap.\n"
             "pub capability Demo.Clock:\n"
             "  def now(): U64\n",
             &nd);
    add_unit(&img,
             "// prompt: wrong return type handler.\n"
             "pub handler Demo.BadClock for Demo.Clock:\n"
             "  def now(): Bool = true\n",
             &nd);
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "typecheck bad handler");
    CHECK(has_type_code(&td, STIGI_TYPE_HANDLER_SIGNATURE), "handler-signature mismatch caught");
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* 7. Evaluator hardening: (-1) < 2^63, U8 overflow, expect(1,2,3) non-crash, string index, mock handler */
  {
    StigiImage img;
    StigiNameDiags nd;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);

    add_unit(&img,
             "// prompt: mixed signed/unsigned comparison.\n"
             "val Demo.Tests.mixedCmp {Stigi.Test}: Stigi.Test.Result =\n"
             "  Stigi.Test.expect(-1 < 9223372036854775808, true, \"-1 < 2^63 must be true\")\n",
             &nd);
    add_unit(&img,
             "// prompt: U8 overflow.\n"
             "val Demo.u8Over: U8 =\n"
             "  val first: U8 = 200\n"
             "  val second: U8 = 200\n"
             "  first + second\n",
             &nd);
    add_unit(&img,
             "// prompt: bad expect reason type (non-crash).\n"
             "val Demo.Tests.badReason {Stigi.Test}: Stigi.Test.Result =\n"
             "  Stigi.Test.expect(1, 2, 3)\n",
             &nd);
    add_unit(&img,
             "// prompt: string literal byte indexing.\n"
             "val Demo.Tests.strIdx {Stigi.Test}: Stigi.Test.Result =\n"
             "  val text: Array[U8](3) = \"abc\"\n"
             "  Stigi.Test.expect(text(1), 'b', \"second byte of abc is b\")\n",
             &nd);
    add_unit(&img,
             "// prompt: counter capability.\n"
             "pub capability Demo.Counter:\n"
             "  def bump(): U64\n",
             &nd);
    add_unit(&img,
             "// prompt: stateful mock handler.\n"
             "pub handler Demo.MockCounter(startVal: U64) for Demo.Counter:\n"
             "  var count: U64 = startVal\n"
             "  def bump(): U64 =\n"
             "    count = count + 1\n"
             "    count\n",
             &nd);
    add_unit(&img,
             "// prompt: test using mock handler.\n"
             "val Demo.Tests.mockRun {Stigi.Test}: Stigi.Test.Result =\n"
             "  handle ctr = Demo.MockCounter(10):\n"
             "    val first: U64 = ctr.bump()\n"
             "    val second: U64 = ctr.bump()\n"
             "    Stigi.Test.expect(first + second, 23, \"11 + 12 == 23\")\n",
             &nd);

    Arena eva;
    arena_init(&eva);

    int32_t i_mix = stigi_image_find(&img, "Demo.Tests.mixedCmp", strlen("Demo.Tests.mixedCmp"));
    StigiEvalResult rm = stigi_eval_test(&img, (uint32_t)i_mix, STIGI_DEFAULT_FUEL, &eva);
    CHECK(rm.status == STIGI_OK && rm.value.kind == STIGI_VAL_TEST_OK,
          "eval: (-1) < 2^63 is true");

    int32_t i_u8 = stigi_image_find(&img, "Demo.u8Over", strlen("Demo.u8Over"));
    StigiEvalResult ru = stigi_eval_unit(&img, (uint32_t)i_u8, STIGI_DEFAULT_FUEL, &eva);
    CHECK(ru.status != STIGI_OK, "eval: U8 200 + 200 overflows cleanly");

    int32_t i_br = stigi_image_find(&img, "Demo.Tests.badReason", strlen("Demo.Tests.badReason"));
    StigiEvalResult rbr = stigi_eval_test(&img, (uint32_t)i_br, STIGI_DEFAULT_FUEL, &eva);
    CHECK(rbr.status != STIGI_OK && rbr.diag_code != NULL && strcmp(rbr.diag_code, "eval-error") == 0,
          "eval: Stigi.Test.expect(1, 2, 3) returns eval-error without crashing");

    int32_t i_si = stigi_image_find(&img, "Demo.Tests.strIdx", strlen("Demo.Tests.strIdx"));
    StigiEvalResult rsi = stigi_eval_test(&img, (uint32_t)i_si, STIGI_DEFAULT_FUEL, &eva);
    CHECK(rsi.status == STIGI_OK && rsi.value.kind == STIGI_VAL_TEST_OK,
          "eval: string literal byte indexing returns 'b'");

    int32_t i_mh = stigi_image_find(&img, "Demo.Tests.mockRun", strlen("Demo.Tests.mockRun"));
    StigiEvalResult rmh = stigi_eval_test(&img, (uint32_t)i_mh, STIGI_DEFAULT_FUEL, &eva);
    CHECK(rmh.status == STIGI_OK && rmh.value.kind == STIGI_VAL_TEST_OK,
          "eval: stateful mock handler inside Test runs and mutates handler var");

    arena_free(&eva);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }
}

static void test_phase2_patterns_multihead_pipe(void) {
  Diag d;
  diag_clear(&d);

  /* 1. S119 runtime_id parity: multi-head def == multi-scrutinee match, and pipe == direct call */
  {
    StigiImage img1, img2;
    StigiNameDiags nd1, nd2;
    stigi_image_init(&img1);
    stigi_image_init(&img2);
    stigi_name_diags_init(&nd1);
    stigi_name_diags_init(&nd2);

    const char *step_u =
        "// prompt: add delta to base.\n"
        "def Demo.addDelta(base: U64, delta: U64): U64 = base + delta\n";
    add_unit(&img1, step_u, &nd1);
    add_unit(&img1,
             "// prompt: multi-head classify.\n"
             "pub def Demo.classify(left: Bool, right: Bool): U64 =\n"
             "  case (true, true) => 3\n"
             "  case (true, false) => 2\n"
             "  case (false, true) => 1\n"
             "  case (false, false) => 0\n",
             &nd1);
    add_unit(&img1,
             "// prompt: piped add.\n"
             "pub def Demo.piped(start: U64): U64 =\n"
             "  start |> Demo.addDelta(5)\n",
             &nd1);
    add_unit(&img1,
             "// prompt: outside reader for pub defs.\n"
             "val Other.Tests.useBoth {Stigi.Test}: Stigi.Test.Result =\n"
             "  Stigi.Test.expect(Demo.classify(true, false) + Demo.piped(1), 8, \"2 + 6 == 8\")\n",
             &nd1);

    add_unit(&img2, step_u, &nd2);
    add_unit(&img2,
             "// prompt: explicit multi-scrutinee classify.\n"
             "pub def Demo.classify(left: Bool, right: Bool): U64 =\n"
             "  (left, right) match\n"
             "    case (true, true) => 3\n"
             "    case (true, false) => 2\n"
             "    case (false, true) => 1\n"
             "    case (false, false) => 0\n",
             &nd2);
    add_unit(&img2,
             "// prompt: direct add.\n"
             "pub def Demo.piped(start: U64): U64 =\n"
             "  Demo.addDelta(start, 5)\n",
             &nd2);
    add_unit(&img2,
             "// prompt: outside reader for pub defs.\n"
             "val Other.Tests.useBoth {Stigi.Test}: Stigi.Test.Result =\n"
             "  Stigi.Test.expect(Demo.classify(true, false) + Demo.piped(1), 8, \"2 + 6 == 8\")\n",
             &nd2);

    CHECK(stigi_image_resolve(&img1, &nd1, &d) == STIGI_OK && nd1.count == 0, "p2 names img1");
    CHECK(stigi_image_resolve(&img2, &nd2, &d) == STIGI_OK && nd2.count == 0, "p2 names img2");

    int32_t c1 = stigi_image_find(&img1, "Demo.classify", strlen("Demo.classify"));
    int32_t c2 = stigi_image_find(&img2, "Demo.classify", strlen("Demo.classify"));
    CHECK(c1 >= 0 && c2 >= 0 && memcmp(img1.units[c1].unit_id, img2.units[c2].unit_id, 32) == 0,
          "S119: multi-head def and multi-scrutinee match have identical runtime_id");

    int32_t p1 = stigi_image_find(&img1, "Demo.piped", strlen("Demo.piped"));
    int32_t p2 = stigi_image_find(&img2, "Demo.piped", strlen("Demo.piped"));
    CHECK(p1 >= 0 && p2 >= 0 && memcmp(img1.units[p1].unit_id, img2.units[p2].unit_id, 32) == 0,
          "S119: pipe |> and direct call have identical runtime_id");

    stigi_name_diags_free(&nd1);
    stigi_name_diags_free(&nd2);
    stigi_image_free(&img1);
    stigi_image_free(&img2);
  }

  /* 2. Maranget exhaustiveness (concrete witness), never-reached, wildcard-arm, and evaluation */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);

    add_unit(&img,
             "// prompt: optional word.\n"
             "enum Demo.Maybe:\n"
             "  case Some(value: U64)\n"
             "  case None\n",
             &nd);
    add_unit(&img,
             "// prompt: pair of optional words.\n"
             "enum Demo.Pair:\n"
             "  case Pair(first: Demo.Maybe, second: Demo.Maybe)\n",
             &nd);
    add_unit(&img,
             "// prompt: double a word.\n"
             "def Demo.double(value: U64): U64 = value + value\n",
             &nd);
    add_unit(&img,
             "// prompt: add two words.\n"
             "def Demo.plus(left: U64, right: U64): U64 = left + right\n",
             &nd);
    add_unit(&img,
             "// prompt: nested match with guard and multi-head def and val-else and pipe.\n"
             "def Demo.score(pair: Demo.Pair): U64 =\n"
             "  case Pair(Some(left), Some(right)) if left < right => right |> Demo.plus(left) |> Demo.double\n"
             "  case Pair(Some(left), Some(right)) => left + right\n"
             "  case Pair(Some(left), None) => left\n"
             "  case Pair(None, Some(right)) => right\n"
             "  case Pair(None, None) => 0\n",
             &nd);
    add_unit(&img,
             "// prompt: val-else chaining.\n"
             "def Demo.addMaybes(first: Demo.Maybe, second: Demo.Maybe): U64 =\n"
             "  val Some(left) = first else\n"
             "    case None => 0\n"
             "  val Some(right) = second else\n"
             "    case None => left\n"
             "  left + right\n",
             &nd);
    add_unit(&img,
             "// prompt: test nested patterns, guards, val-else, pipe.\n"
             "val Demo.Tests.phaseTwoOk {Stigi.Test}: Stigi.Test.Result =\n"
             "  val one: U64 = Demo.score(Pair(Some(2), Some(5)))\n"
             "  val two: U64 = Demo.score(Pair(Some(6), Some(3)))\n"
             "  val three: U64 = Demo.score(Pair(None, Some(4)))\n"
             "  val four: U64 = Demo.addMaybes(Some(10), Some(7))\n"
             "  val five: U64 = Demo.addMaybes(Some(10), None)\n"
             "  val six: U64 = Demo.addMaybes(None, Some(9))\n"
             "  Stigi.Test.expect(one + two + three + four + five + six, 14 + 9 + 4 + 17 + 10 + 0, \"all S121-S123 evals match\")\n",
             &nd);

    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK && nd.count == 0, "p2 eval names ok");
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK && td.count == 0, "p2 eval types ok");
    Arena eva;
    arena_init(&eva);
    int32_t tidx = stigi_image_find(&img, "Demo.Tests.phaseTwoOk", strlen("Demo.Tests.phaseTwoOk"));
    CHECK(tidx >= 0, "find phaseTwoOk");
    StigiEvalResult er = stigi_eval_test(&img, (uint32_t)tidx, STIGI_DEFAULT_FUEL, &eva);
    if (er.status != STIGI_OK) {
      fprintf(stderr, "phaseTwoOk eval failed: code=%s msg=%s\n",
              er.diag_code ? er.diag_code : "NULL", er.msg);
    }
    CHECK(er.status == STIGI_OK && er.value.kind == STIGI_VAL_TEST_OK,
          "Phase 2.2+2.3: nested patterns + guards + multi-head + val-else + pipe evaluate accurately");
    arena_free(&eva);
    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }

  /* 3. Negative checks: not-exhaustive with witness, never-reached, wildcard-arm */
  {
    StigiImage img;
    StigiNameDiags nd;
    StigiTypeDiags td;
    stigi_image_init(&img);
    stigi_name_diags_init(&nd);
    stigi_type_diags_init(&td);

    add_unit(&img,
             "// prompt: optional word.\n"
             "enum Demo.Maybe:\n"
             "  case Some(value: U64)\n"
             "  case None\n",
             &nd);
    add_unit(&img,
             "// prompt: pair.\n"
             "enum Demo.Pair:\n"
             "  case Pair(first: Demo.Maybe, second: Demo.Maybe)\n",
             &nd);
    add_unit(&img,
             "// prompt: missing Pair(None, Some(_)).\n"
             "pub def Demo.missingCase(pair: Demo.Pair): U64 =\n"
             "  pair match\n"
             "    case Pair(Some(_), _) => 1\n"
             "    case Pair(None, None) => 0\n",
             &nd);
    add_unit(&img,
             "// prompt: dead arm.\n"
             "pub def Demo.deadArm(flag: Bool): U64 =\n"
             "  flag match\n"
             "    case true => 1\n"
             "    case false => 0\n"
             "    case true => 2\n",
             &nd);
    add_unit(&img,
             "// prompt: wildcard on closed enum.\n"
             "pub def Demo.wildEnum(item: Demo.Maybe): U64 =\n"
             "  item match\n"
             "    case Some(value) => value\n"
             "    case _ => 0\n",
             &nd);

    CHECK(stigi_image_resolve(&img, &nd, &d) == STIGI_OK, "p2 neg names");
    CHECK(stigi_typecheck_image(&img, &td, &d) == STIGI_OK, "p2 neg types");
    bool got_non_exh = false, got_witness = false, got_dead = false, got_wild = false;
    for (uint32_t i = 0; i < td.count; i++) {
      if (td.items[i].code == STIGI_TYPE_NOT_EXHAUSTIVE) {
        got_non_exh = true;
        if (strstr(td.items[i].msg, "Pair") != NULL &&
            strstr(td.items[i].msg, "None") != NULL &&
            strstr(td.items[i].msg, "Some") != NULL) {
          got_witness = true;
        }
      }
      if (td.items[i].code == STIGI_TYPE_NEVER_REACHED) got_dead = true;
      if (td.items[i].code == STIGI_TYPE_WILDCARD_ARM) got_wild = true;
    }
    CHECK(got_non_exh && got_witness, "Maranget reports not-exhaustive with concrete witness Pair(None, Some(_))");
    CHECK(got_dead, "Maranget reports never-reached on shadowed arm");
    CHECK(got_wild, "Maranget reports wildcard-arm on closed enum");

    stigi_type_diags_free(&td);
    stigi_name_diags_free(&nd);
    stigi_image_free(&img);
  }
}

int main(void) {
  test_names_lookup_and_id_invariance();
  test_alpha_equiv_and_dep_propagation();
  test_names_diagnostics();
  test_linear_normalizer();
  test_typecheck_cases();
  test_evaluator();
  test_pro_review_regressions();
  test_wave2_hardening();
  test_phase2_patterns_multihead_pipe();

  if (g_failures == 0) {
    printf("c3_c4_test: ALL PASS\n");
    return 0;
  }
  fprintf(stderr, "c3_c4_test: %d FAILURES\n", g_failures);
  return 1;
}
