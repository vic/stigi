#define _POSIX_C_SOURCE 200809L

#include "ast.h"
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "print.h"
#include "tenet_gate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                           \
  do {                                                                             \
    if (!(cond)) {                                                                 \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));              \
      g_failures++;                                                                \
    }                                                                              \
  } while (0)

static void check_roundtrip(const char *label, const char *src, StigiDeclKind expected_kind) {
  Arena a;
  arena_init(&a);
  StigiParseResult r = stigi_parse_unit(src, strlen(src), &a);
  if (r.status != STIGI_OK || r.unit == NULL) {
    fprintf(stderr, "FAIL roundtrip[%s] parse: %s (%s)\n", label, r.msg,
            r.diag_code != NULL ? r.diag_code : "none");
    g_failures++;
    arena_free(&a);
    return;
  }
  if (r.unit->decl->kind != expected_kind) {
    fprintf(stderr, "FAIL roundtrip[%s] decl kind: got %d expected %d\n", label,
            (int)r.unit->decl->kind, (int)expected_kind);
    g_failures++;
  }
  char *printed = NULL;
  size_t plen = 0;
  Diag d;
  diag_clear(&d);
  Status st = stigi_print_unit(r.unit, &printed, &plen, &a, &d);
  if (st != STIGI_OK) {
    fprintf(stderr, "FAIL roundtrip[%s] print: %s\n", label, d.msg);
    g_failures++;
    arena_free(&a);
    return;
  }
  if (plen != strlen(src) || memcmp(printed, src, plen) != 0) {
    fprintf(stderr, "FAIL roundtrip[%s] mismatch:\n--- WANT ---\n%s--- GOT ---\n%s---\n",
            label, src, printed);
    g_failures++;
  }
  arena_free(&a);
}

static void test_roundtrips(void) {
  check_roundtrip(
      "def_generic_evidence",
      "// prompt: the element at a proved index.\n"
      "def Demo.Table.at [A: Type] (count: U64, table: Array[A](count), index: U64) <proof: below(index, count)>: A =\n"
      "  table(index)\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "def_exit_facts",
      "// prompt: the larger of two words.\n"
      "pub def Demo.larger(left: U64, right: U64): U64 <lessOrEqual(left, result), lessOrEqual(right, result)> =\n"
      "  if left < right then right else left\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "lemma_def",
      "// prompt: transitivity of below.\n"
      "lemma def Demo.belowTrans(low: U64, mid: U64, high: U64) <below(low, mid), below(mid, high)>: <below(low, high)> decreases(high) =\n"
      "  @<below(low, high)>\n",
      STIGI_DECL_LEMMA_DEF);

  check_roundtrip(
      "enum",
      "// prompt: a position inside the variables of one rule.\n"
      "enum Stigi.Datalog.Index:\n"
      "  case VariableIndex(variables: U64, position: U64) <below(position, variables)>\n"
      "  case Constant(value: U64)\n",
      STIGI_DECL_ENUM);

  check_roundtrip(
      "type_refined",
      "// prompt: a decimal digit.\n"
      "type Demo.Digit = U64 <below(self, 10)>\n",
      STIGI_DECL_TYPE);

  check_roundtrip(
      "type_param",
      "// prompt: text of a known size.\n"
      "type Demo.Text (size: U64) = Array[U8](size)\n",
      STIGI_DECL_TYPE);

  check_roundtrip(
      "capability",
      "// prompt: one open file descriptor; it must be closed on every path.\n"
      "capability Stigi.File:\n"
      "  state Open, Closed\n"
      "  start Open\n"
      "  stop Closed\n"
      "  def read <at(self, Open)> (count: U64, block: Array[U8](count)): Stigi.File.Read <at(self, Open)>\n"
      "  def close <at(self, Open)> (): Stigi.File.Close <at(self, Closed)>\n",
      STIGI_DECL_CAPABILITY);

  check_roundtrip(
      "handler",
      "// prompt: a file over one POSIX descriptor.\n"
      "handler Stigi.Posix.File(descriptor: I32) for Stigi.File:\n"
      "  var closed: Bool = false\n"
      "  def read(count: U64, block: Array[U8](count)): Stigi.File.Read = Stigi.Posix.readInto(descriptor, count, block)\n"
      "  def close(): Stigi.File.Close = Stigi.Posix.closeOnce(descriptor)\n",
      STIGI_DECL_HANDLER);

  check_roundtrip(
      "extern",
      "// prompt: POSIX write; reads at most count bytes of the block.\n"
      "extern \"c\" def write(descriptor: I32, block: Array[U8](count), count: U64): I64\n",
      STIGI_DECL_EXTERN);

  check_roundtrip(
      "relation",
      "// prompt: one program says that one resource is the parent of another.\n"
      "relation Demo.child(inner: Stigi.Id.Capability, outer: Stigi.Id.Capability)\n",
      STIGI_DECL_RELATION);

  check_roundtrip(
      "rule",
      "// prompt: below passes through lessOrEqual on its right side.\n"
      "rule def Stigi.Bounds.belowThenLessOrEqual: Stigi.Facts.below(low, high) =\n"
      "  Stigi.Facts.below(low, middle), Stigi.Facts.lessOrEqual(middle, high)\n",
      STIGI_DECL_RULE);

  check_roundtrip(
      "test",
      "// prompt: larger gives the right operand when it is greater.\n"
      "val Demo.Tests.largerRight {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(Demo.larger(2, 5), 5, \"the right operand is greater\")\n",
      STIGI_DECL_TEST);

  check_roundtrip(
      "neg_test",
      "// prompt: an index with no bound is rejected.\n"
      "val Demo.NegTests.indexWithoutBound {Stigi.NegTest}: Never =\n"
      "  def at(count: U64, table: Array[U64](count), index: U64): U64 = table(index)\n"
      "  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, \"no fact bounds the index\")\n",
      STIGI_DECL_NEG_TEST);

  check_roundtrip(
      "while_decreases_var_inv",
      "// prompt: how many bytes of a block equal one byte.\n"
      "def Demo.Bytes.count(size: U64, block: Array[U8](size), wanted: U8): U64 <lessOrEqual(result, size)> =\n"
      "  var matches@<lessOrEqual(matches, size)>: U64 = 0\n"
      "  var index: U64 = 0\n"
      "  while index < size decreases(size - index) do\n"
      "    @<lessOrEqual(matches, index), lessOrEqual(index, size)>\n"
      "    matches = if block(index) == wanted then matches + 1 else matches\n"
      "    index = index + 1\n"
      "  matches\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "match_handle",
      "// prompt: handle a region and match an index.\n"
      "alias Idx = Stigi.Datalog.Index\n"
      "def Demo.inspect(item: Idx, chunkBytes: U64): U64 =\n"
      "  handle region = Stigi.Arena(chunkBytes)\n"
      "  item match\n"
      "    case VariableIndex(variables, position) => position\n"
      "    case Constant(value) => value\n",
      STIGI_DECL_DEF);
}

static bool has_gate_code(const StigiGateFindings *f, StigiGateCode code) {
  for (uint32_t i = 0; i < f->count; i++) {
    if (f->items[i].code == code) return true;
  }
  return false;
}

static void check_gate_pos(const char *label, const char *src) {
  StigiGateFindings f;
  stigi_gate_findings_init(&f);
  Diag d;
  diag_clear(&d);
  Status st = stigi_gate_check_source(src, strlen(src), &f, &d);
  CHECK(st == STIGI_OK, label);
  if (f.count != 0) {
    fprintf(stderr, "FAIL gate POS[%s]: expected 0 findings, got %u (first: %s: %s)\n",
            label, f.count, stigi_gate_code_name(f.items[0].code), f.items[0].msg);
    g_failures++;
  }
  stigi_gate_findings_free(&f);
}

static void check_gate_neg(const char *label, const char *src, StigiGateCode expected) {
  StigiGateFindings f;
  stigi_gate_findings_init(&f);
  Diag d;
  diag_clear(&d);
  Status st = stigi_gate_check_source(src, strlen(src), &f, &d);
  CHECK(st == STIGI_OK, label);
  if (!has_gate_code(&f, expected)) {
    fprintf(stderr, "FAIL gate NEG[%s]: expected '%s', got %u findings\n",
            label, stigi_gate_code_name(expected), f.count);
    for (uint32_t i = 0; i < f.count; i++) {
      fprintf(stderr, "  got: %s: %s\n", stigi_gate_code_name(f.items[i].code), f.items[i].msg);
    }
    g_failures++;
  }
  stigi_gate_findings_free(&f);
}

static void test_tenet_gate(void) {
  /* Self-check word table */
  CHECK(stigi_gate_word_count() == 28, "28 general forbidden words");
  CHECK(stigi_gate_exit_word_count() == 4, "4 exit call-target words");
  char wbuf[32];
  stigi_gate_word_decode(0, wbuf, sizeof wbuf);
  CHECK(strcmp(wbuf, "abort") == 0, "first forbidden word decodes to abort");
  stigi_gate_word_decode(27, wbuf, sizeof wbuf);
  CHECK(strcmp(wbuf, "panicked") == 0, "last forbidden word decodes to panicked");
  stigi_gate_exit_word_decode(0, wbuf, sizeof wbuf);
  CHECK(strcmp(wbuf, "exit") == 0, "first exit word decodes to exit");
  stigi_gate_exit_word_decode(3, wbuf, sizeof wbuf);
  CHECK(strcmp(wbuf, "exiting") == 0, "last exit word decodes to exiting");

  /* POS: lateral, trapezoid, stubborn, bytesAt, sizeOf, exitState, freeAtExit -> 0 findings */
  check_gate_pos(
      "pos_names_and_s117",
      "// prompt: compute lateral trapezoid stubborn bytesAt sizeOf exitState freeAtExit.\n"
      "def Demo.lateral(trapezoid: U64, stubborn: U64, bytesAt: U64, sizeOf: U64, exitState: U64, freeAtExit: U64): U64 =\n"
      "  trapezoid + stubborn + bytesAt + sizeOf + exitState + freeAtExit\n");

  check_gate_pos(
      "pos_refined_ptr",
      "// prompt: a bounded pointer inside Stigi.\n"
      "type Stigi.RawBuf = Ptr[U8](16) <valid(self)>\n");

  /* NEG: abort */
  check_gate_neg(
      "neg_abort",
      "// prompt: calls a banned word.\n"
      "def Demo.run(value: U64): U64 =\n"
      "  abort(value)\n",
      STIGI_GATE_FORBIDDEN_WORD);

  /* NEG: isTodoItem */
  check_gate_neg(
      "neg_isTodoItem",
      "// prompt: camelCase forbidden sub-word.\n"
      "def Demo.isTodoItem(value: U64): Bool =\n"
      "  value == 0\n",
      STIGI_GATE_FORBIDDEN_WORD);

  /* NEG: bi */
  check_gate_neg(
      "neg_bi",
      "// prompt: cryptic two-letter name.\n"
      "def Demo.compute(bi: U64): U64 =\n"
      "  bi\n",
      STIGI_GATE_CRYPTIC_NAME);

  /* NEG: x2 */
  check_gate_neg(
      "neg_x2",
      "// prompt: trailing digit name.\n"
      "def Demo.compute(x2: U64): U64 =\n"
      "  x2\n",
      STIGI_GATE_CRYPTIC_NAME);

  /* NEG: bare Ptr */
  check_gate_neg(
      "neg_bare_ptr",
      "// prompt: bare pointer in def.\n"
      "def Demo.read(pointer: Ptr[U8](8)): U64 =\n"
      "  0\n",
      STIGI_GATE_BARE_POINTER);

  check_gate_neg(
      "neg_unrefined_ptr_type",
      "// prompt: pointer in type without fact group.\n"
      "type Demo.Raw = Ptr[U8](8)\n",
      STIGI_GATE_BARE_POINTER);

  /* NEG: 2-arg Stigi.Test.expect */
  check_gate_neg(
      "neg_expect_2args",
      "// prompt: test with only two args.\n"
      "val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(1, 1)\n",
      STIGI_GATE_EXPECT_FORM);

  /* NEG: empty reason */
  check_gate_neg(
      "neg_expect_empty_reason",
      "// prompt: test with empty reason.\n"
      "val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(1, 1, \"\")\n",
      STIGI_GATE_EXPECT_FORM);

  /* NEG: missing // prompt: */
  check_gate_neg(
      "neg_prompt_missing",
      "def Demo.identity(value: U64): U64 =\n"
      "  value\n",
      STIGI_GATE_PROMPT_MISSING);

  /* NEG: S117 call exit(0), exit[U64](0), exit{region}(0) (Fix #4) */
  check_gate_neg(
      "neg_exit_call_s117",
      "// prompt: calls exit directly.\n"
      "def Demo.stopNow(): I32 =\n"
      "  exit(0)\n",
      STIGI_GATE_FORBIDDEN_WORD);
  check_gate_neg(
      "neg_exit_generic_call_s117",
      "// prompt: calls generic exit.\n"
      "def Demo.stopGeneric(): I32 =\n"
      "  exit[U64](0)\n",
      STIGI_GATE_FORBIDDEN_WORD);
  check_gate_neg(
      "neg_exit_cap_call_s117",
      "// prompt: calls capability exit.\n"
      "def Demo.stopCap(): I32 =\n"
      "  exit{region}(0)\n",
      STIGI_GATE_FORBIDDEN_WORD);

  /* NEG: cryptic-name in constructor & match arm binders (Fix #4) */
  check_gate_neg(
      "neg_val_ctor_binder",
      "// prompt: cryptic binder in val ctor pattern.\n"
      "def Demo.unpack(pair: Demo.Pair): U64 =\n"
      "  val Pair(bi, x2) = pair\n"
      "  bi\n",
      STIGI_GATE_CRYPTIC_NAME);
  check_gate_neg(
      "neg_match_ctor_binder",
      "// prompt: cryptic binder in match arm pattern.\n"
      "def Demo.inspect(pair: Demo.Pair): U64 =\n"
      "  pair match\n"
      "    case Pair(bi, x2) => bi\n",
      STIGI_GATE_CRYPTIC_NAME);

  /* Fix #6: char/string escape decoding and u64 literal overflow detection */
  {
    Arena pa;
    arena_init(&pa);
    const char *lit_src =
        "// prompt: escaped byte and string literals.\n"
        "def Demo.esc(): U8 =\n"
        "  val msg: Array[U8](3) = \"a\\nb\"\n"
        "  '\\n'\n";
    StigiParseResult pr = stigi_parse_unit(lit_src, strlen(lit_src), &pa);
    CHECK(pr.status == STIGI_OK && pr.unit != NULL, "parse escape literals");
    if (pr.unit != NULL) {
      const StigiExpr *blk = pr.unit->decl->u.def_decl.body;
      CHECK(blk->u.block.stmts[0]->u.val_stmt.init->u.lit_str.decoded_len == 3,
            "\"a\\nb\" decoded_len == 3");
      CHECK(blk->u.block.stmts[1]->u.lit_char.value == (uint8_t)'\n',
            "'\\n' decodes to 10");
    }
    const char *ov_src =
        "// prompt: overflowing u64 literal.\n"
        "val Demo.huge: U64 = 99999999999999999999999\n";
    StigiParseResult ovr = stigi_parse_unit(ov_src, strlen(ov_src), &pa);
    CHECK(ovr.status == STIGI_OK && ovr.unit->decl->u.val_decl.body->u.lit_int.overflow,
          "u64 overflow literal flagged");
    arena_free(&pa);
  }

  /* NEG: name-case */
  check_gate_neg(
      "neg_name_case",
      "// prompt: uppercase def name.\n"
      "def Demo.BadName(value: U64): U64 =\n"
      "  value\n",
      STIGI_GATE_NAME_CASE);

  /* S117 on emitted C source */
  {
    StigiGateFindings f;
    stigi_gate_findings_init(&f);
    Diag d;
    diag_clear(&d);
    const char *c_ok = "uint64_t exitState = 0; uint64_t freeAtExit = exitState;\n";
    CHECK(stigi_gate_check_c_source(c_ok, strlen(c_ok), &f, &d) == STIGI_OK, "c_ok");
    CHECK(f.count == 0, "c_ok has 0 findings under S117");
    stigi_gate_findings_free(&f);

    stigi_gate_findings_init(&f);
    const char *c_bad = "int main(void) { exit(1); }\n";
    CHECK(stigi_gate_check_c_source(c_bad, strlen(c_bad), &f, &d) == STIGI_OK, "c_bad");
    CHECK(has_gate_code(&f, STIGI_GATE_FORBIDDEN_WORD), "c_bad triggers S117 forbidden-word");
    stigi_gate_findings_free(&f);
  }

  /* case-pair check */
  {
    StigiGateFindings f;
    stigi_gate_findings_init(&f);
    Diag d;
    diag_clear(&d);
    const char *names[] = {"Demo.Alpha", "Demo.alpha", "Demo.Beta"};
    CHECK(stigi_gate_check_case_pairs(names, 3, &f, &d) == STIGI_OK, "case_pair");
    CHECK(has_gate_code(&f, STIGI_GATE_CASE_PAIR), "case_pair detected");
    stigi_gate_findings_free(&f);
  }
}

static void check_not_in_lang(const char *label, const char *src) {
  Arena a;
  arena_init(&a);
  StigiParseResult r = stigi_parse_unit(src, strlen(src), &a);
  if (r.diag_kind != STIGI_PARSE_ERR_NOT_IN_LANG ||
      r.diag_code == NULL || strcmp(r.diag_code, "not-in-language") != 0 ||
      r.help == NULL || r.help[0] == '\0') {
    fprintf(stderr, "FAIL not-in-language[%s]: kind=%d code=%s help=%s msg=%s\n",
            label, (int)r.diag_kind,
            r.diag_code != NULL ? r.diag_code : "NULL",
            r.help != NULL ? r.help : "NULL", r.msg);
    g_failures++;
  }
  arena_free(&a);
}

static void test_not_in_language(void) {
  check_not_in_lang("class", "// prompt: uses class.\nclass Demo.Box\n");
  check_not_in_lang("throw", "// prompt: uses throw.\ndef Demo.fail(): U64 =\n  throw 1\n");
  check_not_in_lang("null", "// prompt: uses null.\nval Demo.empty: U64 = null\n");
  check_not_in_lang("return", "// prompt: uses return.\ndef Demo.one(): U64 =\n  return 1\n");
  check_not_in_lang("import", "// prompt: uses import.\nimport Demo.Other\ndef Demo.one(): U64 = 1\n");
  check_not_in_lang("float", "// prompt: uses float.\nval Demo.half: U64 = 0.5\n");
  check_not_in_lang("extern_abi", "// prompt: bad abi.\nextern \"rust\" def foo(): U64\n");
}

static void bench_lexer_1mb(void) {
  const char *sample =
      "// prompt: how many bytes of a block equal one byte.\n"
      "def Demo.Bytes.count(size: U64, block: Array[U8](size), wanted: U8): U64 <lessOrEqual(result, size)> =\n"
      "  var matches: U64 = 0\n"
      "  var index: U64 = 0\n"
      "  while index < size decreases(size - index) do\n"
      "    @<lessOrEqual(matches, index), lessOrEqual(index, size)>\n"
      "    matches = if block(index) == wanted then matches + 1 else matches\n"
      "    index = index + 1\n"
      "  matches\n";
  size_t slen = strlen(sample);
  size_t target = 1024U * 1024U;
  size_t total = ((target + slen - 1U) / slen) * slen;
  char *buf = malloc(total + 1U);
  CHECK(buf != NULL, "bench malloc");
  if (buf == NULL) return;
  for (size_t off = 0; off < total; off += slen) {
    memcpy(buf + off, sample, slen);
  }
  buf[total] = '\0';

  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  StigiLexer lex;
  stigi_lex_init(&lex, buf, total);
  uint64_t tok_count = 0;
  for (;;) {
    StigiTok t = stigi_lex_next(&lex);
    tok_count++;
    if (t.kind == TOK_EOF || t.kind == TOK_ERROR) break;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);

  double elapsed_s = (double)(t1.tv_sec - t0.tv_sec) +
                     (double)(t1.tv_nsec - t0.tv_nsec) * 1e-9;
  double mb = (double)total / (1024.0 * 1024.0);
  double mb_per_s = elapsed_s > 0.0 ? mb / elapsed_s : 0.0;
  printf("c2_test: lex bench: %.2f MB in %.4f s (%.1f MB/s, %llu tokens)\n",
         mb, elapsed_s, mb_per_s, (unsigned long long)tok_count);
  CHECK(mb_per_s >= 20.0, "lexer throughput >= 20 MB/s");
  free(buf);
}

int main(void) {
  test_roundtrips();
  test_tenet_gate();
  test_not_in_language();
  bench_lexer_1mb();

  if (g_failures == 0) {
    printf("c2_test: ALL PASS\n");
    return 0;
  }
  fprintf(stderr, "c2_test: %d FAILURES\n", g_failures);
  return 1;
}
