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

int main(void) {
  test_names_lookup_and_id_invariance();
  test_alpha_equiv_and_dep_propagation();
  test_names_diagnostics();
  test_linear_normalizer();
  test_typecheck_cases();
  test_evaluator();
  test_pro_review_regressions();

  if (g_failures == 0) {
    printf("c3_c4_test: ALL PASS\n");
    return 0;
  }
  fprintf(stderr, "c3_c4_test: %d FAILURES\n", g_failures);
  return 1;
}
