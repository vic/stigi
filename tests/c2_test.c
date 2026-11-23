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
  /* S129 pattern sugar: alternatives (also nested), byte and int ranges, text patterns, `is` */
  check_roundtrip(
      "pattern_sugar",
      "// prompt: classify a shape and a byte with pattern sugar.\n"
      "pub def Demo.kind(shape: Demo.Shape, code: U8, count: U64, word: Array[U8](count)): U64 =\n"
      "  val flat: Bool = shape is Dot | Line(_)\n"
      "  val digit: Bool = code is '0'..'9' | 'a'..'f'\n"
      "  val pair: Bool = shape is Box(1 | 2, _) && code is 0..9\n"
      "  val other: Bool = !(code is 'a'..'z') && !(shape is Dot)\n"
      "  word match\n"
      "    case \"yes\" | \"y\" => 1\n"
      "    case _ => if flat && digit || pair || other then 2 else 3\n",
      STIGI_DECL_DEF);

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
      "lemma def Demo.belowTrans(low: U64, mid: U64, high: U64) <below(low, mid), below(mid, high), decreases(high)>: <below(low, high)> =\n"
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
      "nested_match_and_guard",
      "// prompt: nested constructor pattern with guard.\n"
      "def Demo.unpack(pair: Demo.Pair): U64 =\n"
      "  pair match\n"
      "    case Pair(Some(value), flag) if value > 0 => value\n"
      "    case Pair(Some(_), _) => 0\n"
      "    case Pair(None, _) => 1\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "multi_scrutinee_match",
      "// prompt: two-scrutinee tuple match.\n"
      "def Demo.bothFlags(left: Bool, right: Bool): U64 =\n"
      "  (left, right) match\n"
      "    case (true, true) => 3\n"
      "    case (true, false) => 2\n"
      "    case (false, true) => 1\n"
      "    case (false, false) => 0\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "multi_head_def",
      "// prompt: Erlang-style multi-head function.\n"
      "def Demo.classify(first: Bool, second: Bool): U64 =\n"
      "  case (true, true) => 3\n"
      "  case (true, false) => 2\n"
      "  case (false, true) => 1\n"
      "  case (false, false) => 0\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "val_else_and_pipe",
      "// prompt: flat exhaustive val-else binding and pipe.\n"
      "def Demo.pipeline(input: Demo.Maybe): U64 =\n"
      "  val Some(value) = input else\n"
      "    case None => 0\n"
      "  value |> Demo.step(1) |> Demo.double\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "trait_and_given",
      "// prompt: structural equality trait protocol.\n"
      "pub trait Demo.Equal [A: Type]:\n"
      "  def equal(left: A, right: A): Bool\n",
      STIGI_DECL_TRAIT);

  check_roundtrip(
      "parametric_given",
      "// prompt: recursive equality given for optional values.\n"
      "pub given Demo.Equal.Maybe [A: Type] {Demo.Equal[A]}: Demo.Equal[Demo.Maybe[A]]:\n"
      "  def equal(left: Demo.Maybe[A], right: Demo.Maybe[A]): Bool =\n"
      "    case (Some(first), Some(second)) => Demo.Equal.equal(first, second)\n"
      "    case (None, None) => true\n"
      "    case (Some(_), None) => false\n"
      "    case (None, Some(_)) => false\n",
      STIGI_DECL_GIVEN);

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
      "  while index < size do\n"
      "    @<lessOrEqual(matches, index), lessOrEqual(index, size), decreases(size - index)>\n"
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

  check_roundtrip(
      "denial_multi_atom",
      "// prompt: count rows and finish statement before closing database.\n"
      "def Demo.Database.count {database: Demo.Database} <at(database, Open)> (query: Demo.Text(size), size: U64): U64 <at(database, Closed)> =\n"
      "  handle statement = Demo.Statement(database, size, query)\n"
      "  @<not (close(database), at(statement, Prepared))>\n"
      "  val rows = statement.step()\n"
      "  statement.finish()\n"
      "  database.close()\n"
      "  rows\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "marker_capability",
      "// prompt: marker capability for mutation.\n"
      "capability Stigi.Mut:\n",
      STIGI_DECL_CAPABILITY);

  check_roundtrip(
      "cap_using_handler_and_generic_handle",
      "// prompt: handler using another capability for generic capability.\n"
      "handler Demo.Buffered [A: Type] {region: Stigi.Region} (cap: U64) for Demo.Channel[A]:\n"
      "  def send(item: A): Unit = ()\n",
      STIGI_DECL_HANDLER);

  check_roundtrip(
      "generic_expr_no_parens",
      "// prompt: C-DSL size of element type.\n"
      "extern \"c\" def Stigi.Array.bytesFor [A: Type] (count: U64) <below(count, Stigi.Array.maximumCount[A])>: U64 =\n"
      "  count * size[A]\n",
      STIGI_DECL_EXTERN);

  check_roundtrip(
      "precedence_and_multiline_if_then",
      "// prompt: preserve precedence and multiline if-then-match.\n"
      "def Demo.prec(flag: Bool, left: U64, right: U64): U64 =\n"
      "  val negFlag: Bool = !(left == right)\n"
      "  val scaled: U64 = (if flag then left else right) + 3\n"
      "  if negFlag then\n"
      "    scaled match\n"
      "      case 0 => 1\n"
      "      case _ => scaled\n"
      "  else\n"
      "    left + right\n",
      STIGI_DECL_DEF);

  check_roundtrip(
      "rule_neg_lit_and_parens",
      "// prompt: rule with negative literal and parenthesized arithmetic.\n"
      "rule def Stigi.Arith.sample: Stigi.Facts.ok(out) =\n"
      "  Stigi.Facts.val(num), num > -1, out = num * (num + 1)\n",
      STIGI_DECL_RULE);

  check_roundtrip(
      "bool_pattern_match",
      "// prompt: match over Bool.\n"
      "def Demo.fromBool(flag: Bool): U64 =\n"
      "  flag match\n"
      "    case true => 1\n"
      "    case false => 0\n",
      STIGI_DECL_DEF);

  /* Verify ALL 3 real D1 units verbatim from vic/plan/d1-units.md parse, pass tenet-gate, and round-trip idempotently */
  const char *d1_units[] = {
      "// prompt: outcome of appending one row into a heap-backed table.\n"
      "enum Stigi.Table.AppendResult[A: Type]:\n"
      "  case Appended(cap: U64, count: U64, items: Array[A](cap)) <lessOrEqual(count, cap), below(0, cap)>\n"
      "  case AppendFailed(cap: U64, count: U64, items: Array[A](cap)) <lessOrEqual(count, cap), below(0, cap)>\n",

      "// prompt: append one row into a heap-backed table, growing capacity by doubling via Array.tryGrow when full.\n"
      "pub def Stigi.Table.append [A: Type] {region: Stigi.Region, Stigi.Mut} (\n"
      "    cap: U64,\n"
      "    count: U64,\n"
      "    items: Array[A](cap),\n"
      "    row: A\n"
      ") <lessOrEqual(count, cap), below(0, cap), below(cap, Stigi.Table.maxCap)> : Stigi.Table.AppendResult[A] =\n"
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
      "        AppendResult.AppendFailed(cap, count, kept)\n",

      "// prompt: scan one identifier span forward in source starting at start.\n"
      "pub def Stigi.Lex.step(\n"
      "    len: U64,\n"
      "    start: U64,\n"
      "    source: Array[U8](len)\n"
      ") <lessOrEqual(start, len)> : U64 <lessOrEqual(start, result), lessOrEqual(result, len)> =\n"
      "  var pos: U64 = start\n"
      "  while pos < len && Stigi.Lex.isIdentByte(source(pos)) do\n"
      "    @<lessOrEqual(start, pos), lessOrEqual(pos, len), decreases(len - pos)>\n"
      "    pos = pos + 1\n"
      "  pos\n",

      "// prompt: allocate bytes with 8-byte alignment from an arena chunk, or map a new chunk via Array.tryFill.\n"
      "pub def Stigi.Arena.tryAllocate {region: Stigi.Region} (\n"
      "    cap: U64,\n"
      "    pos: U64,\n"
      "    bytes: U64,\n"
      "    chunk: Array[U8](cap)\n"
      ") <lessOrEqual(pos, cap), below(cap, Stigi.Arena.maxChunk), below(bytes, Stigi.Arena.maxChunk)> : Stigi.Arena.AllocResult =\n"
      "  val rem: U64 = bytes % 8\n"
      "  val pad: U64 = (8 - rem) % 8\n"
      "  val aligned: U64 = bytes + pad\n"
      "  val nextPos: U64 = pos + aligned\n"
      "  if nextPos <= cap then\n"
      "    AllocResult.Allocated(pos, nextPos, cap, chunk)\n"
      "  else\n"
      "    val chunkCap: U64 = if aligned < 65536 then 65536 else aligned\n"
      "    Array.tryFill{region}(chunkCap, 0) match\n"
      "      case Filled(freshChunk) =>\n"
      "        AllocResult.Allocated(0, aligned, chunkCap, freshChunk)\n"
      "      case FillFailed =>\n"
      "        AllocResult.FillFailed\n"
  };
  for (size_t i = 0; i < sizeof(d1_units) / sizeof(d1_units[0]); i++) {
    Arena a;
    arena_init(&a);
    StigiGateFindings gf;
    stigi_gate_findings_init(&gf);
    Diag gd;
    diag_clear(&gd);
    CHECK(stigi_gate_check_source(d1_units[i], strlen(d1_units[i]), &gf, &gd) == STIGI_OK && gf.count == 0,
          "D1 unit passes tenet gate");
    if (gf.count > 0) {
      fprintf(stderr, "  D1[%zu] gate finding: %s: %s\n", i,
              stigi_gate_code_name(gf.items[0].code), gf.items[0].msg);
    }
    stigi_gate_findings_free(&gf);

    StigiParseResult r1 = stigi_parse_unit(d1_units[i], strlen(d1_units[i]), &a);
    if (r1.status != STIGI_OK || r1.unit == NULL) {
      fprintf(stderr, "  D1[%zu] parse error at %u:%u: %s (%s)\n", i,
              r1.err_span.line, r1.err_span.col, r1.msg,
              r1.diag_code != NULL ? r1.diag_code : "none");
    }
    CHECK(r1.status == STIGI_OK && r1.unit != NULL, "D1 unit parses");
    if (r1.unit != NULL) {
      char *p1 = NULL, *p2 = NULL;
      size_t l1 = 0, l2 = 0;
      CHECK(stigi_print_unit(r1.unit, &p1, &l1, &a, &gd) == STIGI_OK, "D1 unit prints");
      StigiParseResult r2 = stigi_parse_unit(p1, l1, &a);
      CHECK(r2.status == STIGI_OK && r2.unit != NULL, "D1 unit re-parses");
      if (r2.unit != NULL) {
        CHECK(stigi_print_unit(r2.unit, &p2, &l2, &a, &gd) == STIGI_OK, "D1 unit re-prints");
        CHECK(l1 == l2 && memcmp(p1, p2, l1) == 0, "D1 unit print(parse(print(parse(x)))) idempotent");
      }
    }
    arena_free(&a);
  }
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
  /* Self-check ALL 32 decoded words (28 general + 4 exit) */
  CHECK(stigi_gate_word_count() == 28, "28 general forbidden words");
  CHECK(stigi_gate_exit_word_count() == 4, "4 exit call-target words");
  static const char *const WANT_WORDS[28] = {
      "abort", "aborts", "aborted", "aborting",
      "trap", "traps", "trapped", "trapping",
      "unreachable",
      "todo", "todos",
      "fixme", "fixmes",
      "skip", "skips", "skipped", "skipping",
      "later",
      "stub", "stubs", "stubbed", "stubbing",
      "placeholder", "placeholders",
      "unsupported",
      "panic", "panics", "panicked"
  };
  static const char *const WANT_EXITS[4] = { "exit", "exits", "exited", "exiting" };
  char wbuf[32];
  for (size_t i = 0; i < 28; i++) {
    stigi_gate_word_decode(i, wbuf, sizeof wbuf);
    CHECK(strcmp(wbuf, WANT_WORDS[i]) == 0, "forbidden word table entry matches spec");
  }
  for (size_t i = 0; i < 4; i++) {
    stigi_gate_exit_word_decode(i, wbuf, sizeof wbuf);
    CHECK(strcmp(wbuf, WANT_EXITS[i]) == 0, "exit word table entry matches spec");
  }

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

  check_gate_neg(
      "neg_ptr_lt_inside_len_bypass",
      "// prompt: pointer with '<' inside len expr, no fact group.\n"
      "type Demo.Raw = Ptr[U8](if 1 < 2 then 8 else 8)\n",
      STIGI_GATE_BARE_POINTER);

  check_gate_pos(
      "pos_multiline_refined_ptr",
      "// prompt: multiline refined pointer type.\n"
      "type Stigi.RawBuf =\n"
      "  Ptr[U8](16)\n"
      "  <valid(self)>\n");

  /* NEG: 2-arg Stigi.Test.expect, alias bypass, whitespace reason, NegTest.expect */
  check_gate_neg(
      "neg_expect_2args",
      "// prompt: test with only two args.\n"
      "val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(1, 1)\n",
      STIGI_GATE_EXPECT_FORM);

  check_gate_neg(
      "neg_expect_empty_reason",
      "// prompt: test with empty reason.\n"
      "val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(1, 1, \"\")\n",
      STIGI_GATE_EXPECT_FORM);

  check_gate_neg(
      "neg_expect_whitespace_reason",
      "// prompt: test with whitespace-only reason.\n"
      "val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(1, 1, \"   \\t \")\n",
      STIGI_GATE_EXPECT_FORM);

  check_gate_neg(
      "neg_expect_alias_bypass",
      "// prompt: aliased expect with only two args.\n"
      "alias checkEqual = Stigi.Test.expect\n"
      "val Demo.Tests.bad {Stigi.Test}: Stigi.Test.Result =\n"
      "  checkEqual(1, 1)\n",
      STIGI_GATE_EXPECT_FORM);

  check_gate_pos(
      "pos_expect_alias_valid",
      "// prompt: aliased expect with valid 3 args.\n"
      "alias checkEqual = Stigi.Test.expect\n"
      "val Demo.Tests.good {Stigi.Test}: Stigi.Test.Result =\n"
      "  checkEqual(1, 1, \"one equals one\")\n");

  check_gate_neg(
      "neg_negtest_expect_1arg",
      "// prompt: negtest expect missing reason.\n"
      "val Demo.NegTests.bad {Stigi.NegTest}: Never =\n"
      "  Stigi.NegTest.expect(Stigi.Code.IndexUnproved)\n",
      STIGI_GATE_EXPECT_FORM);

  /* NEG: cryptic-name & name-case on alias, handle binder, cap binder, evidence binder, type param, underscore */
  check_gate_neg(
      "neg_alias_cryptic",
      "// prompt: cryptic alias name.\n"
      "alias bi = Demo.Table.at\n"
      "def Demo.one(): U64 = 1\n",
      STIGI_GATE_CRYPTIC_NAME);

  check_gate_neg(
      "neg_handle_binder_cryptic",
      "// prompt: cryptic handle binder.\n"
      "def Demo.one(bytes: U64): U64 =\n"
      "  handle r = Stigi.Arena(bytes)\n"
      "  1\n",
      STIGI_GATE_CRYPTIC_NAME);

  check_gate_neg(
      "neg_cap_binder_cryptic",
      "// prompt: cryptic cap binder.\n"
      "def Demo.one {r: Stigi.Region} (): U64 = 1\n",
      STIGI_GATE_CRYPTIC_NAME);

  check_gate_neg(
      "neg_evidence_binder_cryptic",
      "// prompt: cryptic evidence binder.\n"
      "def Demo.one(idx: U64, len: U64) <p: below(idx, len)>: U64 = idx\n",
      STIGI_GATE_CRYPTIC_NAME);

  check_gate_neg(
      "neg_type_param_digit",
      "// prompt: type param ending in digit.\n"
      "def Demo.one [T2: Type] (value: T2): T2 = value\n",
      STIGI_GATE_CRYPTIC_NAME);

  check_gate_neg(
      "neg_underscore_in_def",
      "// prompt: underscore inside L0 def name.\n"
      "def Demo.bad_name(value: U64): U64 = value\n",
      STIGI_GATE_NAME_CASE);

  check_gate_pos(
      "pos_extern_c_underscore_clock_gettime",
      "// prompt: POSIX clock_gettime extern.\n"
      "extern \"c\" def clock_gettime(clock: I32, time: Array[U8](16)): I32\n");

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

    /* Hardened C gate bypass checks: newline, block comment, parenthesized callee, string literal */
    const char *c_bypasses[] = {
        "int main(void) { exit\n(1); }\n",
        "int main(void) { exit /* gap */ (1); }\n",
        "int main(void) { (exit)(1); }\n",
        "int main(void) { __builtin_unreachable(); }\n"
    };
    for (size_t i = 0; i < sizeof(c_bypasses) / sizeof(c_bypasses[0]); i++) {
      stigi_gate_findings_init(&f);
      CHECK(stigi_gate_check_c_source(c_bypasses[i], strlen(c_bypasses[i]), &f, &d) == STIGI_OK,
            "c_bypass scan");
      CHECK(has_gate_code(&f, STIGI_GATE_FORBIDDEN_WORD), "c_bypass triggers forbidden-word");
      stigi_gate_findings_free(&f);
    }

    stigi_gate_findings_init(&f);
    const char *c_str_ok = "const char *msg = \"exit(1) inside literal\";\n";
    CHECK(stigi_gate_check_c_source(c_str_ok, strlen(c_str_ok), &f, &d) == STIGI_OK, "c_str_ok");
    CHECK(f.count == 0, "exit(1) inside C string literal does not false-positive");
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
  check_not_in_lang("lambda_expr", "// prompt: uses lambda.\ndef Demo.fn(val1: U64): U64 = (num: U64) => num + 1\n");
  check_not_in_lang("lambda_bare", "// prompt: uses bare lambda.\ndef Demo.fn(): U64 = num => num + 1\n");
  check_not_in_lang("string_interp", "// prompt: uses string interpolation.\nval Demo.msg: Array[U8](2) = s\"hi\"\n");
  check_not_in_lang("type_lambda", "// prompt: uses type lambda.\ntype Demo.Id = [A] =>> A\n");
  check_not_in_lang("varargs", "// prompt: uses varargs.\ndef Demo.many(items: U64*): U64 = 0\n");
  check_not_in_lang("acell", "// prompt: uses ACell.\ntype Demo.Shared = ACell[U64]\n");

  /* Verify S56 Stigi.Help rows exist for Option, Vec.push, Str, recursive-type */
  CHECK(strstr(stigi_help_not_in_language("Option"), "enum") != NULL, "Stigi.Help Option");
  CHECK(strstr(stigi_help_not_in_language("Vec.push"), "Array.tryGrow") != NULL, "Stigi.Help Vec.push");
  CHECK(strstr(stigi_help_not_in_language("Str"), "Array[U8]") != NULL, "Stigi.Help Str");
  CHECK(strstr(stigi_help_not_in_language("recursive-type"), "Array") != NULL, "Stigi.Help recursive-type");

  /* Hardened lexer rejections: bad escapes, raw control chars, numeric suffix, leading/trailing _ */
  const char *bad_lex[] = {
      "// prompt: bad escape.\nval Demo.msg: Array[U8](1) = \"\\q\"\n",
      "// prompt: raw tab in string.\nval Demo.msg: Array[U8](1) = \"\t\"\n",
      "// prompt: numeric suffix.\nval Demo.num: U64 = 123foo\n",
      "// prompt: numeric keyword suffix.\nval Demo.num: U64 = 123then\n",
      "// prompt: bad hex suffix.\nval Demo.num: U64 = 0x1g\n",
      "// prompt: leading underscore.\ndef Demo._bad(): U64 = 1\n",
      "// prompt: trailing underscore.\ndef Demo.bad_(): U64 = 1\n"
  };
  for (size_t i = 0; i < sizeof(bad_lex) / sizeof(bad_lex[0]); i++) {
    Arena a;
    arena_init(&a);
    StigiParseResult r = stigi_parse_unit(bad_lex[i], strlen(bad_lex[i]), &a);
    CHECK(r.status != STIGI_OK, "bad lex rejected");
    arena_free(&a);
  }

  /* Depth cap: 300 nested parens fails cleanly with parse instead of crashing */
  {
    char deep[800];
    size_t pos = (size_t)snprintf(deep, sizeof deep, "// prompt: deep.\nval Demo.deep: U64 = ");
    for (int i = 0; i < 300; i++) deep[pos++] = '(';
    deep[pos++] = '1';
    for (int i = 0; i < 300; i++) deep[pos++] = ')';
    deep[pos++] = '\n';
    deep[pos] = '\0';
    Arena a;
    arena_init(&a);
    StigiParseResult r = stigi_parse_unit(deep, pos, &a);
    CHECK(r.status != STIGI_OK && r.diag_code != NULL && strcmp(r.diag_code, "parse") == 0,
          "deep nesting hits STIGI_MAX_PARSE_DEPTH cleanly");
    arena_free(&a);
  }
}

static void bench_lexer_1mb(void) {
  const char *sample =
      "// prompt: how many bytes of a block equal one byte.\n"
      "def Demo.Bytes.count(size: U64, block: Array[U8](size), wanted: U8): U64 <lessOrEqual(result, size)> =\n"
      "  var matches: U64 = 0\n"
      "  var index: U64 = 0\n"
      "  while index < size do\n"
      "    @<lessOrEqual(matches, index), lessOrEqual(index, size), decreases(size - index)>\n"
      "    matches = if block(index) == wanted then matches + 1 else matches\n"
      "    index = index + 1\n"
      "  matches\n";
  size_t slen = strlen(sample);
  size_t iters = (1024U * 1024U + slen - 1U) / slen;
  size_t total = iters * slen;

  Arena a;
  arena_init(&a);
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  uint64_t tok_count = 0;
  for (size_t i = 0; i < iters; i++) {
    arena_reset(&a);
    StigiTok *toks = NULL;
    uint32_t ntoks = 0;
    Diag d;
    diag_clear(&d);
    CHECK(stigi_lex_all(sample, slen, &toks, &ntoks, &a, &d) == STIGI_OK, "bench lex");
    StigiGateFindings gf;
    stigi_gate_findings_init(&gf);
    CHECK(stigi_gate_check_tokens(toks, ntoks, &gf, &d) == STIGI_OK && gf.count == 0, "bench gate");
    stigi_gate_findings_free(&gf);
    StigiParseResult pr = stigi_parse_tokens(toks, ntoks, &a);
    CHECK(pr.status == STIGI_OK && pr.unit != NULL, "bench parse");
    tok_count += ntoks;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  arena_free(&a);

  double elapsed_s = (double)(t1.tv_sec - t0.tv_sec) +
                     (double)(t1.tv_nsec - t0.tv_nsec) * 1e-9;
  double mb = (double)total / (1024.0 * 1024.0);
  double mb_per_s = elapsed_s > 0.0 ? mb / elapsed_s : 0.0;
  printf("c2_test: lex+gate+parse bench: %.2f MB in %.4f s (%.1f MB/s, %llu tokens)\n",
         mb, elapsed_s, mb_per_s, (unsigned long long)tok_count);
#if defined(__SANITIZE_ADDRESS__) || defined(__FILC__)
  CHECK(mb_per_s >= 1.5, "lex+gate+parse throughput >= 1.5 MB/s under ASan/Fil-C");
#else
  CHECK(mb_per_s >= 10.0, "lex+gate+parse throughput >= 10 MB/s");
#endif
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
