#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include "door.h"
#include "emit.h"
#include "eval.h"
#include "tenet_gate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_failures = 0;

#define CHECK(cond, msg)                                                           \
  do {                                                                             \
    if (!(cond)) {                                                                 \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));              \
      g_failures++;                                                                \
    }                                                                              \
  } while (0)

static int run_cmd(const char *cmd) {
  int rc = system(cmd);
  if (rc == -1) return -1;
  return WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
}

static char *read_text_file(const char *path) {
  FILE *f = fopen(path, "r");
  if (f == NULL) return NULL;
  size_t cap = 65536U;
  size_t len = 0;
  char *buf = malloc(cap);
  if (buf == NULL) {
    (void)fclose(f);
    return NULL;
  }
  for (;;) {
    if (len + 4096U + 1U > cap) {
      cap *= 2U;
      char *nb = realloc(buf, cap);
      if (nb == NULL) {
        free(buf);
        (void)fclose(f);
        return NULL;
      }
      buf = nb;
    }
    size_t n = fread(buf + len, 1, 4096U, f);
    len += n;
    if (n < 4096U) break;
  }
  buf[len] = '\0';
  (void)fclose(f);
  return buf;
}

/* write emitted C to path and compile it with the strict clang line, then remove it */
static bool clang_compiles(const char *c_src, size_t len, const char *path) {
  FILE *f = fopen(path, "w");
  if (f == NULL) return false;
  bool ok = fwrite(c_src, 1, len, f) == len;
  ok = (fclose(f) == 0) && ok;
  char cmd[512];
  (void)snprintf(cmd, sizeof cmd,
                 "clang -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-strict-aliasing -fwrapv "
                 "-fno-delete-null-pointer-checks -c -o /dev/null %s",
                 path);
  ok = ok && run_cmd(cmd) == 0;
  (void)unlink(path);
  return ok;
}

static bool has_spectre_mask_insn(const char *asm_text) {
  if (asm_text == NULL) return false;
#if defined(__x86_64__)
  return strstr(asm_text, "sbbq") != NULL;
#elif defined(__aarch64__)
  return strstr(asm_text, "csdb") != NULL;
#else
  return true;
#endif
}

int main(void) {
  StigiDoor door;
  Diag d;
  diag_clear(&d);

  if (stigi_door_init(&door, "vic/spec/datalog/all.dl",
                      STIGI_DEFAULT_ROW_BUDGET, true, &d) != STIGI_OK) {
    fprintf(stderr, "FAIL stigi_door_init: %s\n", d.msg);
    return 1;
  }

  /* Complete L0 program covering:
   * - mixed-alignment enum + Array-field enum (_Static_assert sizeof/alignOf/offsetof)
   * - match (value & void), signed negative literals (I32)
   * - guarded array read, tainted read, dominating-guard tainted read, tainted write
   * - tail-rec (value & void), lemma erasure, Test, main */
  const char *u_file =
      "// prompt: kernel file capability.\n"
      "pub capability Stigi.File:\n"
      "  def read(count: U64): U64\n";

  const char *u_mut =
      "// prompt: kernel mut marker capability.\n"
      "pub capability Stigi.Mut:\n"
      "  def writeMarker(): Unit\n";

  const char *u_enum =
      "// prompt: a tagged number or empty marker.\n"
      "pub enum Demo.Tag:\n"
      "  case Number(value: U64)\n"
      "  case Mixed(head: U8, mid: U32, tail: U8)\n"
      "  case Empty\n";

  const char *u_buf_enum =
      "// prompt: an enum holding an Array field to lock 1-word nullable-pointer layout.\n"
      "pub enum Demo.BufferWrap:\n"
      "  case Slice(data: Array[U8](4))\n"
      "  case Absent\n";

  const char *u_tail_enum =
      "// prompt: enum whose tag packs into 7 bytes of tail padding (sizeof == 16).\n"
      "pub enum Demo.TailPack:\n"
      "  case Wide(word: U64, flag: U8)\n"
      "  case EmptyPack\n";

  const char *u_region =
      "// prompt: kernel region capability.\n"
      "pub capability Stigi.Region:\n"
      "  def regionMarker(): Unit\n";

  const char *u_alloc_use =
      "// prompt: allocate a 64-byte cache-line aligned stack buffer via Array.tryFill.\n"
      "pub def Demo.allocAndRead {region: Stigi.Region} (slot: U64) <below(slot, 64)>: U8 =\n"
      "  Array.tryFill{region}(64, 'Z') match\n"
      "    case Filled(buffer) => buffer(slot)\n"
      "    case FillFailed => 0\n";

  const char *u_alloc_dyn =
      "// prompt: allocate and deterministically free a dynamic 64-byte aligned buffer.\n"
      "def Demo.allocDynamic {region: Stigi.Region} (count: U64) <below(0, count)>: U8 =\n"
      "  Array.tryFill{region}(count, 'Y') match\n"
      "    case Filled(buffer) => buffer(0)\n"
      "    case FillFailed => 0\n";

  const char *u_alloc_loop =
      "// prompt: run repeated dynamic alloc and free cycles in a bounded loop.\n"
      "pub def Demo.allocLoop {region: Stigi.Region} (rounds: U64, count: U64) <below(0, count)>: U8 =\n"
      "  var step: U64 = 0\n"
      "  var latest: U8 = 0\n"
      "  while step < rounds do\n"
      "    @<lessOrEqual(step, rounds)>\n"
      "    latest = Demo.allocDynamic(count)\n"
      "    step = step + 1\n"
      "  latest\n";

  const char *u_extract =
      "// prompt: extract value from tag enum.\n"
      "pub def Demo.extractTag(item: Demo.Tag): U64 =\n"
      "  item match\n"
      "    case Number(value) => value\n"
      "    case Mixed(_, _, _) => 1\n"
      "    case Empty => 0\n";

  const char *u_extract_tail =
      "// prompt: extract word from tail-packed enum using expression-position and integer match.\n"
      "pub def Demo.extractTail(packed: Demo.TailPack): U64 =\n"
      "  val chosen: U64 = packed match\n"
      "    case Wide(word, flag) =>\n"
      "      flag match\n"
      "        case 1 => word\n"
      "        case _ => 0\n"
      "    case EmptyPack => 0\n"
      "  chosen\n";

  const char *u_signed =
      "// prompt: signed negative literal comparison.\n"
      "pub def Demo.signCheck(value: I32): I32 =\n"
      "  if value < 0 then -1 else 1\n";

  const char *u_at =
      "// prompt: untainted guarded byte read.\n"
      "pub def Demo.byteAt(count: U64, items: Array[U8](count), index: U64) <below(index, count)>: U8 =\n"
      "  items(index)\n";

  const char *u_tainted =
      "// prompt: tainted guarded byte read from file capability.\n"
      "pub def Demo.taintedAt {file: Stigi.File} (count: U64, items: Array[U8](count), index: U64) <below(index, count)>: U8 =\n"
      "  items(index)\n";

  const char *u_tainted_guarded =
      "// prompt: dominating-guard tainted byte read (X22-g).\n"
      "pub def Demo.taintedGuarded {file: Stigi.File} (count: U64, items: Array[U8](count), index: U64) <lessOrEqual(0, count)>: U8 =\n"
      "  if index < count then items(index) else 0\n";

  const char *u_tainted_write =
      "// prompt: tainted guarded byte write (Defect 6).\n"
      "def Demo.taintedWrite {file: Stigi.File, mut: Stigi.Mut} (count: U64, items: Array[U8](count), index: U64, byteVal: U8) <below(index, count)>: Unit =\n"
      "  items(index) := byteVal\n";

  const char *u_apply_wrap =
      "// prompt: void match over BufferWrap (Defects 3 and 4).\n"
      "pub def Demo.applyWrap {file: Stigi.File, mut: Stigi.Mut} (wrap: Demo.BufferWrap, slot: U64) <below(slot, 4)>: Unit =\n"
      "  wrap match\n"
      "    case Slice(data) => Demo.taintedWrite(4, data, slot, 1)\n"
      "    case Absent => ()\n";

  const char *u_tailsum_ok =
      "// prompt: tail-recursive countdown.\n"
      "pub def Demo.countDown(limit: U64) <lessOrEqual(0, limit)>: U64 =\n"
      "  if limit < 1 then 0 else Demo.countDown(limit - 1)\n";

  const char *u_void_tail =
      "// prompt: void tail-recursive loop (Defect 4).\n"
      "pub def Demo.voidCount(limit: U64) <lessOrEqual(0, limit)>: Unit =\n"
      "  if limit < 1 then () else Demo.voidCount(limit - 1)\n";

  /* Q0c: a scanner with an array index, a decreases measure and a tail call under a nested if is one loop */
  const char *u_find_scan =
      "// prompt: index of the first byte equal to target at or after pos, or len when none.\n"
      "pub def Demo.findByte(len: U64, data: Array[U8](len), pos: U64, remaining: U64, target: U8) "
      "<lessOrEqual(pos, len), decreases(remaining)>: U64 =\n"
      "  if remaining > 0 && pos < len then\n"
      "    if data(pos) == target then pos else Demo.findByte(len, data, pos + 1, remaining - 1, target)\n"
      "  else len\n";

  /* Q0b (S129): alternatives, byte and int ranges, text patterns and `is`, run in the evaluator (Test) and in the
   * gcc/clang binaries (main): both must give the written answers */
  const char *u_sugar_shape =
      "// prompt: a drawn shape.\n"
      "pub enum Demo.Sugar.Shape:\n"
      "  case Dot\n"
      "  case Line(len: U64)\n"
      "  case Box(wide: U64, high: U64)\n";
  const char *u_sugar_digit =
      "// prompt: the value of a decimal digit byte, ten when the byte is no digit.\n"
      "pub def Demo.Sugar.digitValue(code: U8): U64 <lessOrEqual(0, result)> =\n"
      "  code match\n"
      "    case '0'..'9' => U64.ofU8(code - 48)\n"
      "    case _ => 10\n";
  const char *u_sugar_small =
      "// prompt: whether a byte value is a single decimal digit value.\n"
      "pub def Demo.Sugar.smallDigit(value: U8): Bool =\n"
      "  value is 0..9\n";
  const char *u_sugar_flat =
      "// prompt: whether a shape has no area.\n"
      "pub def Demo.Sugar.isFlat(shape: Demo.Sugar.Shape): Bool =\n"
      "  shape match\n"
      "    case Dot | Line(_) => true\n"
      "    case Box(_, _) => false\n";
  const char *u_sugar_line =
      "// prompt: whether a shape is a line.\n"
      "pub def Demo.Sugar.isLine(shape: Demo.Sugar.Shape): Bool =\n"
      "  shape is Line(_)\n";
  const char *u_sugar_yes =
      "// prompt: one when the word is the text yes, else zero.\n"
      "pub def Demo.Sugar.isYes(count: U64, word: Array[U8](count)): U64 <lessOrEqual(result, 1)> =\n"
      "  word match\n"
      "    case \"yes\" => 1\n"
      "    case _ => 0\n";
  /* Maranget 2008 sec. 4: a guarded literal arm and a tuple row ahead of range rows; the rows after them are
   * specialized, so no decided test is compiled (branch-decided / never-reached before the fix) */
  const char *u_sugar_guard =
      "// prompt: two for a line comment start, one for a word byte, three for a digit, zero else.\n"
      "pub def Demo.Sugar.byteClass(byte: U8, after: U8): U64 <lessOrEqual(result, 3)> =\n"
      "  byte match\n"
      "    case '/' if after == '/' => 2\n"
      "    case 'a'..'z' | 'A'..'Z' | '_' => 1\n"
      "    case '0'..'9' => 3\n"
      "    case _ => 0\n";
  const char *u_sugar_pair =
      "// prompt: one for a lowercase byte tagged one, two for a digit, zero else.\n"
      "pub def Demo.Sugar.pairClass(byte: U8, tag: U64): U64 <lessOrEqual(result, 2)> =\n"
      "  case ('a'..'z', 1) => 1\n"
      "  case ('0'..'9', _) => 2\n"
      "  case (_, _) => 0\n";
  /* nested specialization: the second guarded range is tested under the first one's narrowed `_` end */
  const char *u_sugar_band =
      "// prompt: the first band of three overlapping value bands that holds the value, zero for none.\n"
      "pub def Demo.Sugar.bandClass(value: U8, low: Bool, high: Bool): U64 <lessOrEqual(result, 3)> =\n"
      "  value match\n"
      "    case 0..20 if low => 1\n"
      "    case 10..30 if high => 2\n"
      "    case 15..25 => 3\n"
      "    case _ => 0\n";
  /* emitter: an `is` (a match block) on the right of `&&` / `||` in a loop condition needs statements; it is
   * hoisted under the short-circuit, never emitted as a constant */
  const char *u_sugar_run =
      "// prompt: the count of leading bytes of a text that are lowercase letters or digits.\n"
      "pub def Demo.Sugar.wordRun(count: U64, text: Array[U8](count)): U64 <lessOrEqual(result, count)> =\n"
      "  var idx: U64 = 0\n"
      "  while idx < count && (text(idx) is 'a'..'z' || text(idx) is '0'..'9') do\n"
      "    @<lessOrEqual(idx, count)>\n"
      "    idx = idx + 1\n"
      "  idx\n";
  /* emitter: in a tail loop, a match arm that ends in a nested match gets a dead `break;` so the C compilers report no
   * fall-through (-Werror) */
  const char *u_sugar_walk =
      "// prompt: the sum of two tagged numbers, swapping the tags past mixed ones, zero for an empty tag.\n"
      "pub def Demo.Sugar.tagWalk(tag: Demo.Tag, other: Demo.Tag, limit: U64) <decreases(limit)>: U64 =\n"
      "  if limit < 1 then 0 else\n"
      "    tag match\n"
      "      case Number(value) =>\n"
      "        other match\n"
      "          case Number(more) => if value < 100 && more < 100 then value + more else 0\n"
      "          case Mixed(_, _, _) => Demo.Sugar.tagWalk(other, tag, limit - 1)\n"
      "          case Empty => 0\n"
      "      case Mixed(_, _, _) => Demo.Sugar.tagWalk(other, tag, limit - 1)\n"
      "      case Empty => 0\n";
  /* emitter.md 3: a generic C-DSL unit is emitted once per used element type, with C types from the call site
   * (one call on U8 arrays, one on U32 arrays: both C compilers must accept the emitted C with -Werror) */
  const char *u_copy_run =
      "// prompt: copy the first elements of one array into another until either ends, returning the copied count.\n"
      "pub extern \"c\" def Stigi.Probe.copyRun [A: Type] {Stigi.Mut} (srcCap: U64, src: Array[A](srcCap), srcEnd: U64, "
      "dstCap: U64, dst: Array[A](dstCap)) <lessOrEqual(srcEnd, srcCap)>: U64 <lessOrEqual(result, dstCap)> =\n"
      "  var idx: U64 = 0\n"
      "  while idx < srcEnd && idx < dstCap do\n"
      "    @<lessOrEqual(idx, srcEnd), lessOrEqual(idx, dstCap)>\n"
      "    dst(idx) := src(idx)\n"
      "    idx = idx + 1\n"
      "  idx\n";
  const char *u_copy_both =
      "// prompt: three when three bytes and three words copy into fresh arrays with their last elements intact, else zero.\n"
      "pub def Demo.Sugar.copyBoth {region: Stigi.Region, Stigi.Mut} (): U64 <lessOrEqual(result, 4)> =\n"
      "  val bytes: Array[U8](3) = \"abc\"\n"
      "  val Filled(byteOut) = Array.tryFill[U8]{region}(4, 0) else\n"
      "    case FillFailed => 0\n"
      "  val Filled(words) = Array.tryFill[U32]{region}(3, U32.ofU64(7)) else\n"
      "    case FillFailed => 0\n"
      "  val Filled(wordOut) = Array.tryFill[U32]{region}(4, U32.ofU64(0)) else\n"
      "    case FillFailed => 0\n"
      "  val byteCount: U64 = Stigi.Probe.copyRun(3, bytes, 3, 4, byteOut)\n"
      "  val wordCount: U64 = Stigi.Probe.copyRun(3, words, 3, 4, wordOut)\n"
      "  if byteCount == 3 && byteOut(2) == 'c' && wordOut(2) == U32.ofU64(7) then wordCount else 0\n";
  /* emitter: a binder of a nested constructor pattern is a `val` alias of a fresh pattern field; an array alias
   * keeps its array identity, so `cells(0)` emits an index read, never a C call */
  const char *u_held_cells =
      "// prompt: a counted run of words.\n"
      "enum Demo.Sugar.Cells:\n"
      "  case Cells(size: U64, cells: Array[U64](size))\n";
  const char *u_held_holder =
      "// prompt: an optional counted run of words.\n"
      "enum Demo.Sugar.Holder:\n"
      "  case Held(box: Demo.Sugar.Cells)\n"
      "  case Unheld\n";
  const char *u_held_first =
      "// prompt: the first word of a held run through a nested pattern, zero when none.\n"
      "def Demo.Sugar.firstHeld(holder: Demo.Sugar.Holder): U64 <lessOrEqual(0, result)> =\n"
      "  holder match\n"
      "    case Held(Cells(size, cells)) => if 0 < size then cells(0) else 0\n"
      "    case Unheld => 0\n";
  /* a second-level `val` alias and a `var` alias of the nested binder keep the array identity too */
  const char *u_held_again =
      "// prompt: the first word of a held run through a val alias of a nested binder, zero when none.\n"
      "def Demo.Sugar.againHeld(holder: Demo.Sugar.Holder): U64 <lessOrEqual(0, result)> =\n"
      "  holder match\n"
      "    case Held(Cells(size, cells)) =>\n"
      "      val again = cells\n"
      "      if 0 < size then again(0) else 0\n"
      "    case Unheld => 0\n";
  const char *u_held_moved =
      "// prompt: the first word of a held run through a var alias of a nested binder, zero when none.\n"
      "def Demo.Sugar.movedHeld(holder: Demo.Sugar.Holder): U64 <lessOrEqual(0, result)> =\n"
      "  holder match\n"
      "    case Held(Cells(size, cells)) =>\n"
      "      var moved = cells\n"
      "      if 0 < size then moved(0) else 0\n"
      "    case Unheld => 0\n";
  const char *u_held_read =
      "// prompt: seven when the first word of a held run of sevens reads back through a nested pattern and its aliases, else zero.\n"
      "pub def Demo.Sugar.heldFirst {region: Stigi.Region} (): U64 <lessOrEqual(0, result)> =\n"
      "  val Filled(words) = Array.tryFill[U64]{region}(3, 7) else\n"
      "    case FillFailed => 0\n"
      "  val held: Demo.Sugar.Holder = Held(Cells(3, words))\n"
      "  if Demo.Sugar.firstHeld(Unheld) == 0 && Demo.Sugar.againHeld(held) == 7 && Demo.Sugar.movedHeld(held) == 7 then Demo.Sugar.firstHeld(held) else 0\n";
  const char *u_sugar_test =
      "// prompt: check the pattern sugar on fixed inputs in the evaluator.\n"
      "val Demo.Sugar.Tests.sugarSuite {Stigi.Test}: Stigi.Test.Result =\n"
      "  val digitGood: Bool = Demo.Sugar.digitValue('7') == 7 && Demo.Sugar.digitValue('x') == 10\n"
      "  val smallGood: Bool = Demo.Sugar.smallDigit(3) && Demo.Sugar.smallDigit(12) == false\n"
      "  val flatGood: Bool = Demo.Sugar.isFlat(Dot) && Demo.Sugar.isFlat(Line(4)) && Demo.Sugar.isFlat(Box(2, 3)) == false\n"
      "  val lineGood: Bool = Demo.Sugar.isLine(Line(1)) && Demo.Sugar.isLine(Dot) == false\n"
      "  val yesGood: Bool = Demo.Sugar.isYes(3, \"yes\") == 1 && Demo.Sugar.isYes(2, \"no\") == 0\n"
      "  val guardGood: Bool = Demo.Sugar.byteClass('/', '/') == 2 && Demo.Sugar.byteClass('/', 'x') == 0 && Demo.Sugar.byteClass('q', '/') == 1 && Demo.Sugar.byteClass('5', 'x') == 3\n"
      "  val pairGood: Bool = Demo.Sugar.pairClass('q', 1) == 1 && Demo.Sugar.pairClass('q', 0) == 0 && Demo.Sugar.pairClass('7', 0) == 2\n"
      "  val bandGood: Bool = Demo.Sugar.bandClass(5, true, true) == 1 && Demo.Sugar.bandClass(5, false, true) == 0 && Demo.Sugar.bandClass(25, true, true) == 2 && Demo.Sugar.bandClass(18, false, false) == 3 && Demo.Sugar.bandClass(12, false, false) == 0 && Demo.Sugar.bandClass(12, false, true) == 2 && Demo.Sugar.bandClass(40, true, true) == 0\n"
      "  val runGood: Bool = Demo.Sugar.wordRun(6, \"ab1D2c\") == 3 && Demo.Sugar.wordRun(2, \"q7\") == 2 && Demo.Sugar.wordRun(1, \"Q\") == 0\n"
      "  val walkGood: Bool = Demo.Sugar.tagWalk(Number(3), Number(4), 5) == 7 && Demo.Sugar.tagWalk(Mixed(1, 2, 3), Number(9), 5) == 0 && Demo.Sugar.tagWalk(Empty, Number(1), 5) == 0\n"
      "  val copyGood: Bool = Demo.Sugar.copyBoth{Stigi.Region}() == 3\n"
      "  Stigi.Test.expect(digitGood && smallGood && flatGood && lineGood && yesGood && guardGood && pairGood && bandGood && runGood && walkGood && copyGood, true, "
      "\"pattern sugar gives the written answers\")\n";

  const char *u_lemma =
      "// prompt: erased zero bound lemma.\n"
      "pub lemma def Demo.zeroBound(value: U64) <lessOrEqual(0, value)>: <lessOrEqual(0, value)> =\n"
      "  @<lessOrEqual(0, value)>\n";

  const char *u_add_one =
      "// prompt: add offset to bounded value.\n"
      "def Demo.plusBound(value: U64, offset: U64) <below(value, 1000), below(offset, 1000)>: U64 =\n"
      "  value + offset\n";

  const char *u_eq_trait =
      "// prompt: equality trait protocol for C11 direct given dispatch.\n"
      "pub trait Demo.Equal [A: Type]:\n"
      "  def equal(left: A, right: A): Bool\n";

  const char *u_eq_word =
      "// prompt: concrete equality given for U64 words.\n"
      "pub given Demo.Equal.Word: Demo.Equal[U64]:\n"
      "  def equal(left: U64, right: U64): Bool = left == right\n";

  const char *u_multi_head =
      "// prompt: Erlang-style multi-head function with pipe and val-else.\n"
      "pub def Demo.modernFlow {region: Stigi.Region} (item: Demo.Tag, flag: Bool): U64 <lessOrEqual(0, result)> =\n"
      "  case (Number(value), true) if value < 100 =>\n"
      "    val Filled(words) = Array.tryFill{region}(4, value) else\n"
      "      case FillFailed => 0\n"
      "    val elem: U64 = words(2)\n"
      "    if elem < 100 then elem |> Demo.plusBound(10) else 0\n"
      "  case (Number(_), true) => 1\n"
      "  case (Number(_), false) => 2\n"
      "  case (Mixed(_, _, _), _) => 3\n"
      "  case (Empty, _) => 4\n";

  const char *u_test_ok =
      "// prompt: verify countDown, extractTag, modernFlow, and trait given dispatch.\n"
      "val Demo.Tests.checkAll {Stigi.Test}: Stigi.Test.Result =\n"
      "  val flowOut: U64 = Demo.modernFlow{Stigi.Region}(Number(7), true)\n"
      "  val equalMatch: Bool = Demo.Equal.equal(flowOut, 17)\n"
      "  val total: U64 = Demo.countDown(5) + Demo.extractTag(Number(7)) + (if equalMatch then flowOut else 0)\n"
      "  Stigi.Test.expect(total, 24, \"0 + 7 + 17 == 24\")\n";

  const char *u_main_ok =
      "// prompt: program entry exercising all lowered constructs.\n"
      "pub def main {file: Stigi.File, mut: Stigi.Mut, region: Stigi.Region} (): I32 <lessOrEqual(0, result)> =\n"
      "  Demo.zeroBound(5)\n"
      "  Demo.voidCount(3)\n"
      "  Demo.applyWrap(Absent, 0)\n"
      "  val enumArrayGood: Bool = Array.tryFill(4, Number(42)) match\n"
      "    case Filled(tags) => Demo.extractTag(tags(2)) == 42\n"
      "    case FillFailed => false\n"
      "  val modernOut: U64 = Demo.modernFlow(Number(7), true)\n"
      "  val givenEqualGood: Bool = Demo.Equal.equal(modernOut, 17)\n"
      "  val first: U8 = Demo.byteAt(3, \"abc\", 0)\n"
      "  val second: U8 = Demo.taintedAt(3, \"abc\", 1)\n"
      "  val guarded: U8 = Demo.taintedGuarded(3, \"abc\", 2)\n"
      "  val filledByte: U8 = Demo.allocAndRead(0)\n"
      "  val dynamicByte: U8 = Demo.allocLoop(200, 64)\n"
      "  val done: U64 = Demo.countDown(5)\n"
      "  val tagVal: U64 = Demo.extractTag(Number(7))\n"
      "  val tailVal: U64 = Demo.extractTail(Wide(9, 1))\n"
      "  val foundAt: U64 = Demo.findByte(3, \"abc\", 0, 3, 'c')\n"
      "  val sugarGood: Bool = Demo.Sugar.digitValue('7') == 7 && Demo.Sugar.smallDigit(3) && Demo.Sugar.isFlat(Line(4)) && Demo.Sugar.isLine(Line(1)) && Demo.Sugar.isYes(3, \"yes\") == 1 && Demo.Sugar.byteClass('/', '/') == 2 && Demo.Sugar.byteClass('/', 'x') == 0 && Demo.Sugar.pairClass('7', 0) == 2 && Demo.Sugar.pairClass('q', 0) == 0 && Demo.Sugar.bandClass(18, false, false) == 3 && Demo.Sugar.bandClass(25, true, true) == 2 && Demo.Sugar.wordRun(6, \"ab1D2c\") == 3 && Demo.Sugar.tagWalk(Number(3), Number(4), 5) == 7 && Demo.Sugar.copyBoth() == 3 && Demo.Sugar.heldFirst() == 7\n"
      "  val negInput: I32 = -5\n"
      "  val negWant: I32 = -1\n"
      "  val signResult: I32 = Demo.signCheck(negInput)\n"
      "  val signedCompareTrue: Bool = (-5 < 0)\n"
      "  if enumArrayGood && givenEqualGood && done == 0 && tagVal == 7 && tailVal == 9 && foundAt == 2 && sugarGood && first == 'a' && second == 'b' && guarded == 'c' && filledByte == 'Z' && dynamicByte == 'Y' && signResult == negWant && signedCompareTrue then 0 else 1\n";

  const char *all_srcs[47] = {
      u_file,            u_mut,         u_region,        u_enum,        u_buf_enum,
      u_tail_enum,       u_alloc_use,   u_alloc_dyn,     u_alloc_loop,  u_extract,
      u_extract_tail,    u_signed,      u_at,            u_tainted,     u_tainted_guarded,
      u_tainted_write,   u_apply_wrap,  u_tailsum_ok,    u_void_tail,   u_lemma,
      u_add_one,         u_eq_trait,    u_eq_word,       u_multi_head,  u_test_ok,
      u_main_ok,         u_find_scan,   u_sugar_shape,   u_sugar_digit, u_sugar_small,
      u_sugar_flat,      u_sugar_line,  u_sugar_yes,     u_sugar_guard, u_sugar_pair, u_sugar_band, u_sugar_run, u_sugar_walk, u_copy_run, u_copy_both,
      u_held_cells,      u_held_holder, u_held_first,    u_held_again,  u_held_moved, u_held_read,
      u_sugar_test
  };
  size_t all_lens[47];
  for (int i = 0; i < 47; i++) all_lens[i] = strlen(all_srcs[i]);

  StigiDoorResult dr;
  stigi_door_result_init(&dr);
  CHECK(stigi_door_submit(&door, all_srcs, all_lens, 47, &dr, &d) == STIGI_OK, "door submit");
  if (!dr.accepted) {
    for (uint32_t i = 0; i < dr.reject_count; i++) {
      fprintf(stderr, "  reject: subj=%s code=%s msg=%s\n",
              dr.rejects[i].subject, dr.rejects[i].code, dr.rejects[i].msg);
    }
  }
  CHECK(dr.accepted, "C8 program admitted through StigiDoor");
  stigi_door_result_free(&dr);

  /* 1. Emit twice and verify determinism (byte-equal) */
  char *c1 = NULL, *c2 = NULL;
  size_t l1 = 0, l2 = 0;
  StigiEmitStats st1, st2;
  CHECK(stigi_emit_c(&door, STIGI_EMIT_TEST, &c1, &l1, &st1, &d) == STIGI_OK, "emit 1");
  CHECK(stigi_emit_c(&door, STIGI_EMIT_TEST, &c2, &l2, &st2, &d) == STIGI_OK, "emit 2");
  CHECK(c1 != NULL && c2 != NULL && l1 == l2 && memcmp(c1, c2, l1) == 0,
        "determinism: two emissions are byte-equal");
  {
    char *cr = NULL;
    size_t lr = 0;
    StigiEmitStats str;
    CHECK(stigi_emit_c(&door, STIGI_EMIT_RELEASE, &cr, &lr, &str, &d) == STIGI_OK, "emit release");
    CHECK(cr != NULL && strstr(c1, "Tests_checkAll") != NULL && strstr(cr, "Tests_checkAll") == NULL &&
              strstr(cr, "Stigi_main") != NULL,
          "S163: the release profile emits main and no Test unit");
    CHECK(cr != NULL && clang_compiles(cr, lr, "/tmp/stigi-c8-rel.c"),
          "S163: clang compiles the release C with 0 warnings");
    free(cr);
  }
  {
    /* S163: capability used by main through one handler; a second handler reached only from a Test */
    const char *cap_srcs[6] = {
        "// prompt: the mock counter start, reached only from a Test.\n"
        "def Demo.mockStart(): U64 =\n"
        "  10\n",
        "// prompt: counter capability.\n"
        "capability Demo.Counter:\n"
        "  def bump(): U64\n",
        "// prompt: counter handler used only by a Test (handler tag 0 in the test profile).\n"
        "handler Demo.MockCounter(startVal: U64) for Demo.Counter:\n"
        "  def bump(): U64 =\n"
        "    startVal\n",
        "// prompt: bump a counter once.\n"
        "pub def Demo.useCounter {ctr: Demo.Counter} (): U64 <lessOrEqual(0, result)> =\n"
        "  ctr.bump()\n",
        "// prompt: the mock counter bumps by two.\n"
        "val Demo.Tests.mockBump {Stigi.Test}: Stigi.Test.Result =\n"
        "  handle ctr = Demo.MockCounter(Demo.mockStart())\n"
        "  Stigi.Test.expect(Demo.useCounter{ctr}(), 10, \"mock start is 10\")\n",
        "// prompt: counter handler used by main.\n"
        "pub handler Demo.RisingCounter(startVal: U64) for Demo.Counter:\n"
        "  def bump(): U64 =\n"
        "    startVal\n"};
    const char *main_src =
        "// prompt: program entry bumping the up counter.\n"
        "pub def main (): U64 <lessOrEqual(0, result)> =\n"
        "  handle ctr = Demo.RisingCounter(0)\n"
        "  Demo.useCounter{ctr}()\n";
    for (int with_main = 1; with_main >= 0; with_main--) {
      StigiDoor cd;
      Diag cdg;
      diag_clear(&cdg);
      CHECK(stigi_door_init(&cd, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &cdg) ==
                STIGI_OK,
            "cap door init");
      /* without main: only a Test and the def it reads */
      const char *nomain_test =
          "// prompt: the mock start is ten.\n"
          "val Demo.Tests.startTen {Stigi.Test}: Stigi.Test.Result =\n"
          "  Stigi.Test.expect(Demo.mockStart(), 10, \"mock start is 10\")\n";
      const char *srcs[7] = {cap_srcs[0], cap_srcs[1], cap_srcs[2], cap_srcs[3], cap_srcs[4], cap_srcs[5], main_src};
      if (!with_main) srcs[1] = nomain_test;
      size_t lens[7];
      for (int i = 0; i < 7; i++) lens[i] = strlen(srcs[i]);
      StigiDoorResult cr2;
      stigi_door_result_init(&cr2);
      CHECK(stigi_door_submit(&cd, srcs, lens, with_main ? 7U : 2U, &cr2, &cdg) == STIGI_OK &&
                cr2.accepted,
            "cap program admitted");
      for (uint32_t i = 0; i < cr2.reject_count; i++) {
        fprintf(stderr, "  reject: subj=%s code=%s msg=%s\n", cr2.rejects[i].subject,
                cr2.rejects[i].code, cr2.rejects[i].msg);
      }
      stigi_door_result_free(&cr2);
      char *cc = NULL;
      size_t cl = 0;
      StigiEmitStats cs;
      Status est = stigi_emit_c(&cd, STIGI_EMIT_RELEASE, &cc, &cl, &cs, &cdg);
      if (with_main) {
        CHECK(est == STIGI_OK && cc != NULL && strstr(cc, "MockCounter") == NULL && strstr(cc, "Demo_mockStart") == NULL &&
                  strstr(cc, "RisingCounter") != NULL,
              "S163: release keeps main's handler, drops the Test-only handler and the Test-only def");
        CHECK(cc != NULL && clang_compiles(cc, cl, "/tmp/stigi-c8-rel-cap.c"),
              "S163: clang compiles release C with a Test-only handler shaken");
      } else {
        CHECK(est == STIGI_ERR_INVALID && strstr(cdg.msg, "release profile needs main") != NULL,
              "S163: release without main is a typed error, never an empty binary");
      }
      free(cc);
      stigi_door_free(&cd);
    }
  }

  {
    /* types.md 3: a field's length term names the case's fields; a pattern binds them under its own names, so the
     * length of a pattern-bound array (mask bound, tryGrow old length) names the binders. `other` is bound as `size`
     * so a bound naming the field instead of the binder is wrong, not undeclared; a `_` length field is read into a
     * hidden local `_len_size`, so tryGrow still copies the old elements. */
    const char *box_src =
        "// prompt: a box of one dependent-length array.\n"
        "enum Demo.Box:\n"
        "  case Boxed(size: U64, other: U64, cells: Array[U64](size))\n";
    const char *wrap_src =
        "// prompt: a wrapped box.\n"
        "enum Demo.Wrap:\n"
        "  case Wrapped(box: Demo.Box)\n";
    const char *pair_src =
        "// prompt: a run sized by the sum of two counts.\n"
        "enum Demo.Pair:\n"
        "  case Paired(size: U64, other: U64, cells: Array[U64](size + other)) <below(size, 1000), below(other, 1000)>\n";
    const char *test_src =
        "// prompt: the second boxed word is seven.\n"
        "val Demo.Tests.boxSuite {Stigi.Test}: Stigi.Test.Result =\n"
        "  Stigi.Test.expect(Demo.boxSecond{Stigi.Region}(), 7, \"boxed word reads back\")\n";
#define BOX_DRIVE(value)                                                                \
  "// prompt: seven when the second word of a run of sevens reads back, else zero.\n" \
  "def Demo.boxSecond {region: Stigi.Region} (): U64 <lessOrEqual(0, result)> =\n"     \
  "  val Filled(words) = Array.tryFill[U64]{region}(3, 7) else\n"                       \
  "    case FillFailed => 0\n"                                                          \
  "  Demo.cellAt" value "\n"
    struct {
      const char *def;
      const char *drive;
      const char *extra;
      const char *bound;
    } cases[5] = {
        {"// prompt: the cell at a position of a box, or its other number.\n"
         "def Demo.cellAt(box: Demo.Box, at: U64): U64 <lessOrEqual(0, result)> =\n"
         "  box match\n"
         "    case Boxed(count, size, cells) => if at < count then cells(at) else size\n",
         BOX_DRIVE("(Boxed(3, 0, words), 1)"), box_src, "(uint64_t)(count))"},
        {"// prompt: the cell at a position of a box, or its other number.\n"
         "def Demo.cellAt(box: Demo.Box, at: U64): U64 <lessOrEqual(0, result)> =\n"
         "  val Boxed(count, size, cells) = box\n"
         "  if at < count then cells(at) else size\n",
         BOX_DRIVE("(Boxed(3, 0, words), 1)"), box_src, "(uint64_t)(count))"},
        {"// prompt: the cell at a position of a wrapped box, or its other number.\n"
         "def Demo.cellAt(wrap: Demo.Wrap, at: U64): U64 <lessOrEqual(0, result)> =\n"
         "  wrap match\n"
         "    case Wrapped(Boxed(count, size, cells)) => if at < count then cells(at) else size\n",
         BOX_DRIVE("(Wrapped(Boxed(3, 0, words)), 1)"), wrap_src, "(uint64_t)(patField1))"},
        {"// prompt: the cell at a position of a box grown to eight cells, or its other number.\n"
         "def Demo.cellAt {region: Stigi.Region} (box: Demo.Box, at: U64): U64 <lessOrEqual(0, result)> =\n"
         "  box match\n"
         "    case Boxed(_, size, cells) =>\n"
         "      Array.tryGrow{region}(cells, 8, 0) match\n"
         "        case Grown(grown) => if at < 8 then grown(at) else size\n"
         "        case GrowFailed(_) => size\n",
         BOX_DRIVE("{region}(Boxed(3, 0, words), 1)"), box_src, " = (uint64_t)(_len_size);"},
        {"// prompt: the cell at a position of a paired run grown to eight cells, or its other number.\n"
         "def Demo.cellAt {region: Stigi.Region} (pair: Demo.Pair, at: U64): U64 <lessOrEqual(0, result)> =\n"
         "  pair match\n"
         "    case Paired(other, size, cells) =>\n"
         "      Array.tryGrow{region}(cells, 8, 0) match\n"
         "        case Grown(grown) => if at < 8 then grown(at) else size\n"
         "        case GrowFailed(_) => size\n",
         BOX_DRIVE("{region}(Paired(3, 0, words), 1)"), pair_src, " = (uint64_t)((other + size));"}};
#undef BOX_DRIVE
    for (int k = 0; k < 5; k++) {
      StigiDoor bd;
      Diag bdg;
      diag_clear(&bdg);
      CHECK(stigi_door_init(&bd, "vic/spec/datalog/all.dl", STIGI_DEFAULT_ROW_BUDGET, true, &bdg) == STIGI_OK,
            "binder door init");
      const char *srcs[5] = {cases[k].extra, cases[k].def, cases[k].drive, test_src, box_src};
      size_t lens[5];
      for (int i = 0; i < 5; i++) lens[i] = strlen(srcs[i]);
      StigiDoorResult br;
      stigi_door_result_init(&br);
      CHECK(stigi_door_submit(&bd, srcs, lens, k == 2 ? 5U : 4U, &br, &bdg) == STIGI_OK && br.accepted,
            "binder-rename program admitted");
      for (uint32_t i = 0; i < br.reject_count; i++) {
        fprintf(stderr, "  reject[%d]: subj=%s code=%s msg=%s\n", k, br.rejects[i].subject, br.rejects[i].code,
                br.rejects[i].msg);
      }
      stigi_door_result_free(&br);
      char *bc = NULL;
      size_t bl = 0;
      StigiEmitStats bs;
      Status bst = stigi_emit_c(&bd, STIGI_EMIT_TEST, &bc, &bl, &bs, &bdg);
      CHECK(bst == STIGI_OK && bc != NULL, "binder emit");
      if (bst != STIGI_OK) fprintf(stderr, "  binder[%d] emit: %s\n", k, bdg.msg);
      if (getenv("STIGI_C8_BINDER_DUMP") != NULL && bc != NULL) fprintf(stderr, "---- binder[%d]\n%s\n", k, bc);
      CHECK(bc != NULL && strstr(bc, cases[k].bound) != NULL && strstr(bc, "(uint64_t)(size))") == NULL,
            "types.md 3: the length of a pattern-bound array names the binders of its length fields");
      CHECK(bc != NULL && clang_compiles(bc, bl, "/tmp/stigi-c8-binder.c"),
            "clang compiles C for an array bound under renamed binders");
      free(bc);
      stigi_door_free(&bd);
    }
  }

  /* 2. CRASH LINE, tenet gate on emitted C, zero GNU builtins, 1-word nullable-ptr, tail-tag */
  CHECK(c1 != NULL && strstr(c1, "abort(") == NULL, "CRASH LINE: zero abort(");
  CHECK(c1 != NULL && strstr(c1, "exit(") == NULL, "CRASH LINE: zero exit(");
  CHECK(c1 != NULL && strstr(c1, "__builtin_unreachable") == NULL,
        "emitter.md 3: zero GNU builtins (__builtin_unreachable) in emitted C");
  {
    StigiGateFindings gf;
    stigi_gate_findings_init(&gf);
    CHECK(stigi_gate_check_c_source(c1, l1, &gf, &d) == STIGI_OK, "gate check C source ok");
    if (gf.count > 0) {
      for (uint32_t i = 0; i < gf.count; i++) {
        fprintf(stderr, "  c_gate finding: %s\n", gf.items[i].msg);
      }
    }
    CHECK(gf.count == 0, "stigi_gate_check_c_source: 0 findings on emitted C");
    stigi_gate_findings_free(&gf);
  }
  CHECK(c1 != NULL && strstr(c1, "#line 1 \"<stigi-runtime-main>\"") != NULL,
        "Defect 5: #line reset before main wrapper");
  CHECK(c1 != NULL && strstr(c1, "sizeof(Stigi_Demo_BufferWrap) == 8ULL") != NULL,
        "memory.md 8: 1-word nullable-pointer enum optimization (sizeof == 8)");
  CHECK(c1 != NULL && strstr(c1, "sizeof(Stigi_Demo_TailPack) == 16ULL") != NULL &&
        strstr(c1, "offsetof(Stigi_Demo_TailPack, _hdr.tag) == 9ULL") != NULL,
        "memory.md 8: tail-padding tag placement (sizeof == 16, tag at offset 9)");
  CHECK(c1 != NULL && strstr(c1, "offsetof(Stigi_Demo_Tag, u.Mixed.mid) == 0ULL") != NULL &&
        strstr(c1, "offsetof(Stigi_Demo_Tag, u.Mixed.head) == 4ULL") != NULL,
        "Defect 3: _Static_assert(offsetof(...)) emitted for alignment-sorted enum fields");
  CHECK(c1 != NULL && strstr(c1, "stigi_rt_shutdown(&rt_state)") != NULL,
        "Defect 10: guard-paged stack/pool retained until stigi_rt_shutdown");
  CHECK(st1.enums_emitted >= 3, "enums emitted");
  CHECK(st1.defs_emitted >= 9, "defs emitted");
  CHECK(st1.lemmas_erased >= 1, "lemma erased");
  CHECK(st1.spectre_masks >= 3, "Datalog maskIndex masks taintedAt, taintedGuarded, and taintedWrite");
  CHECK(st1.tail_loops >= 3, "tail-recursive defs (value, void, array scanner) lowered to loops");
  {
    /* Q0c: the scanner's definition body (up to its closing brace at column 0) never names itself */
    const char *def = c1 == NULL ? NULL : strstr(c1, "Stigi_Demo_findByte(uint64_t len, uint8_t * data, uint64_t pos, "
                                                      "uint64_t remaining, uint8_t target) {");
    const char *body = def == NULL ? NULL : strchr(def, '{');
    const char *end = body == NULL ? NULL : strstr(body, "\n}\n");
    const char *self = body == NULL ? NULL : strstr(body + 1, "Stigi_Demo_findByte(");
    CHECK(def != NULL && end != NULL, "Q0c: the scanner definition is emitted");
    CHECK(end != NULL && (self == NULL || self > end), "Q0c: the tail-recursive scanner emits no call of itself");
  }
  CHECK(st1.pure_defs >= 4 && st1.parallel_safe_sites >= 4,
        "T18: Datalog purity & parallel-safe call sites detected");
  CHECK(st1.stack_objects >= 1 && st1.free_at_exit >= 1,
        "T15: constant-size stack promotion & dynamic deterministic free emitted");
  CHECK(c1 != NULL && strstr(c1, "_Alignas(64)") != NULL && strstr(c1, "free(_arr") != NULL,
        "memory.md 1 & 8: 64B cache-aligned stack object and deterministic free in emitted C");

  /* 3. Write /tmp/stigi-c8-out.c and compile with gcc (-fstack-usage), clang, and -DSTIGI_PROFILE_RT_LOCKED */
  FILE *f = fopen("/tmp/stigi-c8-out.c", "w");
  CHECK(f != NULL, "open /tmp/stigi-c8-out.c");
  if (f != NULL && c1 != NULL) {
    CHECK(fwrite(c1, 1, l1, f) == l1, "write emitted C");
    (void)fclose(f);
  }

  int rc_gcc = run_cmd(
      "gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-strict-aliasing -fwrapv "
      "-fno-delete-null-pointer-checks -fstack-usage -c -o /tmp/stigi-c8-out.o /tmp/stigi-c8-out.c && "
      "gcc -o /tmp/stigi-c8-bin /tmp/stigi-c8-out.o");
  CHECK(rc_gcc == 0, "gcc compiles emitted C with 0 warnings");

  int rc_clang = run_cmd(
      "clang -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-strict-aliasing -fwrapv "
      "-fno-delete-null-pointer-checks -o /tmp/stigi-c8-bin-clang /tmp/stigi-c8-out.c");
  CHECK(rc_clang == 0, "clang compiles emitted C with 0 warnings");

  /* Defect 14a: -DSTIGI_PROFILE_RT_LOCKED compile check under -Wall -Wextra -Werror */
  int rc_rt_gcc = run_cmd(
      "gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-strict-aliasing -fwrapv "
      "-fno-delete-null-pointer-checks -DSTIGI_PROFILE_RT_LOCKED -c -o /tmp/stigi-c8-rt-gcc.o "
      "/tmp/stigi-c8-out.c");
  CHECK(rc_rt_gcc == 0, "gcc compiles -DSTIGI_PROFILE_RT_LOCKED with 0 warnings");

  int rc_rt_clang = run_cmd(
      "clang -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-strict-aliasing -fwrapv "
      "-fno-delete-null-pointer-checks -DSTIGI_PROFILE_RT_LOCKED -c -o /tmp/stigi-c8-rt-clang.o "
      "/tmp/stigi-c8-out.c");
  CHECK(rc_rt_clang == 0, "clang compiles -DSTIGI_PROFILE_RT_LOCKED with 0 warnings");

  /* 4. Armed -O2 Spectre disassembly oracle (X22-g) on BOTH GCC and Clang (Defect 14b & 14c) */
  int rc_asm_gcc = run_cmd(
      "gcc -std=c11 -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks "
      "-S -o /tmp/stigi-c8-out-gcc.s /tmp/stigi-c8-out.c");
  CHECK(rc_asm_gcc == 0, "gcc -O2 -S emitted assembly");
  char *gcc_asm = read_text_file("/tmp/stigi-c8-out-gcc.s");
  CHECK(has_spectre_mask_insn(gcc_asm),
        "X22-g GCC Spectre disassembly oracle: mask instruction survives -O2");
  free(gcc_asm);

  int rc_asm_clang = run_cmd(
      "clang -std=c11 -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks "
      "-S -o /tmp/stigi-c8-out-clang.s /tmp/stigi-c8-out.c");
  CHECK(rc_asm_clang == 0, "clang -O2 -S emitted assembly");
  char *clang_asm = read_text_file("/tmp/stigi-c8-out-clang.s");
  CHECK(has_spectre_mask_insn(clang_asm),
        "X22-g Clang Spectre disassembly oracle: mask instruction survives -O2");
  free(clang_asm);

  /* Defect 14c & 14d: Dominating-guard A/B test (non-constant runtime args):
   * (A) Emitted Stigi_mask_index under `if (idx < count)` MUST survive -O2 in GCC and Clang.
   * (B) Plain-C mutant without Stigi_mask_index MUST be folded away (oracle FAILS) in GCC and Clang. */
  {
    const char *guarded_c =
        "#include <stdint.h>\n"
        "static inline uint64_t Stigi_mask_index(uint64_t idx, uint64_t len) {\n"
        "#if defined(__x86_64__)\n"
        "  uint64_t mask;\n"
        "  __asm__ __volatile__(\"cmpq %2, %1\\n\\tsbbq %0, %0\\n\\t\" : \"=r\"(mask) : \"r\"(idx), \"r\"(len) : \"cc\");\n"
        "  return idx & mask;\n"
        "#elif defined(__aarch64__)\n"
        "  uint64_t mask;\n"
        "  __asm__ __volatile__(\"cmp %1, %2\\n\\tcsetm %0, lo\\n\\tcsdb\\n\\t\" : \"=r\"(mask) : \"r\"(idx), \"r\"(len) : \"cc\");\n"
        "  return idx & mask;\n"
        "#else\n"
        "  volatile uint64_t v_idx = idx; volatile uint64_t v_len = len;\n"
        "  uint64_t c_idx = v_idx; uint64_t c_len = v_len;\n"
        "  uint64_t mask = (uint64_t)0 - (uint64_t)(c_idx < c_len);\n"
        "  volatile uint64_t v_mask = mask;\n"
        "  return c_idx & v_mask;\n"
        "#endif\n"
        "}\n"
        "uint8_t tainted_guarded(uint64_t count, const uint8_t *items, uint64_t idx) {\n"
        "  if (idx < count) {\n"
        "    return items[Stigi_mask_index(idx, count)];\n"
        "  }\n"
        "  return 0;\n"
        "}\n";
    FILE *gf = fopen("/tmp/stigi-c8-guarded.c", "w");
    CHECK(gf != NULL, "open /tmp/stigi-c8-guarded.c");
    if (gf != NULL) {
      (void)fwrite(guarded_c, 1, strlen(guarded_c), gf);
      (void)fclose(gf);
    }
    CHECK(run_cmd("gcc -std=c11 -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks "
                  "-S -o /tmp/stigi-c8-guarded-gcc.s /tmp/stigi-c8-guarded.c") == 0,
          "gcc compiles dominating-guard fixture");
    char *gg_asm = read_text_file("/tmp/stigi-c8-guarded-gcc.s");
    CHECK(has_spectre_mask_insn(gg_asm),
          "X22-g GCC dominating-guard fixture retains mask under -O2");
    free(gg_asm);

    CHECK(run_cmd("clang -std=c11 -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks "
                  "-S -o /tmp/stigi-c8-guarded-clang.s /tmp/stigi-c8-guarded.c") == 0,
          "clang compiles dominating-guard fixture");
    char *gc_asm = read_text_file("/tmp/stigi-c8-guarded-clang.s");
    CHECK(has_spectre_mask_insn(gc_asm),
          "X22-g Clang dominating-guard fixture retains mask under -O2");
    free(gc_asm);

    const char *mutant_c =
        "#include <stdint.h>\n"
        "uint8_t tainted_mutant(uint64_t count, const uint8_t *items, uint64_t idx) {\n"
        "  if (idx < count) {\n"
        "    uint64_t masked = idx & ((uint64_t)0 - (uint64_t)(idx < count));\n"
        "    return items[masked];\n"
        "  }\n"
        "  return 0;\n"
        "}\n";
    FILE *mf = fopen("/tmp/stigi-c8-mutant.c", "w");
    CHECK(mf != NULL, "open /tmp/stigi-c8-mutant.c");
    if (mf != NULL) {
      (void)fwrite(mutant_c, 1, strlen(mutant_c), mf);
      (void)fclose(mf);
    }
    int rc_mgcc = run_cmd(
        "gcc -std=c11 -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks "
        "-S -o /tmp/stigi-c8-mutant-gcc.s /tmp/stigi-c8-mutant.c");
    CHECK(rc_mgcc == 0, "gcc compiles plain-C Spectre mutant");
    char *mgcc_asm = read_text_file("/tmp/stigi-c8-mutant-gcc.s");
    CHECK(mgcc_asm != NULL && !has_spectre_mask_insn(mgcc_asm),
          "X22-g ARMED MUTANT (GCC): plain-C mask folded away -> disassembly oracle FAILS");
    free(mgcc_asm);

    int rc_mclang = run_cmd(
        "clang -std=c11 -O2 -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks "
        "-S -o /tmp/stigi-c8-mutant-clang.s /tmp/stigi-c8-mutant.c");
    CHECK(rc_mclang == 0, "clang compiles plain-C Spectre mutant");
    char *mclang_asm = read_text_file("/tmp/stigi-c8-mutant-clang.s");
    CHECK(mclang_asm != NULL && !has_spectre_mask_insn(mclang_asm),
          "X22-g ARMED MUTANT (Clang): plain-C mask folded away -> disassembly oracle FAILS");
    free(mclang_asm);
  }

  /* 5. -fstack-usage gate (positive + negative fail-closed tests for Defects 12 & 13) */
  char su_err[256] = {0};
  Status su_st = stigi_emit_check_su("/tmp/stigi-c8-out.su", &door.image, STIGI_EMIT_TEST, su_err, sizeof su_err);
  if (su_st != STIGI_OK) {
    fprintf(stderr, "FAIL stigi_emit_check_su: %s\n", su_err);
  }
  CHECK(su_st == STIGI_OK, "-fstack-usage gate passes");

  /* Negative .su test 1: empty .su file must fail closed (Defect 12) */
  {
    FILE *ef = fopen("/tmp/stigi-c8-empty.su", "w");
    if (ef != NULL) (void)fclose(ef);
    CHECK(stigi_emit_check_su("/tmp/stigi-c8-empty.su", &door.image, STIGI_EMIT_TEST, su_err, sizeof su_err) != STIGI_OK,
          "Defect 12: empty .su file fails closed (0 matched)");
  }
  /* Negative .su test 2: strict bytes <= fb without +64 slack (Defect 13) */
  {
    FILE *bf = fopen("/tmp/stigi-c8-over.su", "w");
    if (bf != NULL) {
      fprintf(bf, "Demo/countDown.sc:1:1:Stigi_Demo_countDown\t16\tstatic\n");
      (void)fclose(bf);
    }
    CHECK(stigi_emit_check_su("/tmp/stigi-c8-over.su", &door.image, STIGI_EMIT_TEST, su_err, sizeof su_err) != STIGI_OK,
          "Defect 13: strict bytes > fb (16 > 8, no +64 slack) fails closed");
  }
  /* Negative .su test 3: invalid byte count fails strtoull endptr validation (Defect 13) */
  {
    FILE *mf = fopen("/tmp/stigi-c8-badbytes.su", "w");
    if (mf != NULL) {
      fprintf(mf, "Demo/countDown.sc:1:1:Stigi_Demo_countDown\t8xyz\tstatic\n");
      (void)fclose(mf);
    }
    CHECK(stigi_emit_check_su("/tmp/stigi-c8-badbytes.su", &door.image, STIGI_EMIT_TEST, su_err, sizeof su_err) != STIGI_OK,
          "Defect 13: invalid strtoull bytes fails closed");
  }

  /* 6. Value oracle: execute compiled binaries (GCC and Clang) and compare with evaluator */
  int rc_run_gcc = run_cmd("/tmp/stigi-c8-bin");
  CHECK(rc_run_gcc == 0, "GCC compiled binary exits 0 (matches evaluator + embedded Tests pass)");

  int rc_run_clang = run_cmd("/tmp/stigi-c8-bin-clang");
  CHECK(rc_run_clang == 0, "Clang compiled binary exits 0 (matches evaluator + embedded Tests pass)");

  /* S47 Fil-C backstop: when FILCC is set, compile & run emitted C with Fil-C! */
  const char *filcc = getenv("FILCC");
  if (filcc != NULL && filcc[0] != '\0') {
    char cmd[1024];
    (void)snprintf(cmd, sizeof cmd,
                   "%s -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-strict-aliasing "
                   "-fwrapv -fno-delete-null-pointer-checks -o /tmp/stigi-c8-bin-filc "
                   "/tmp/stigi-c8-out.c",
                   filcc);
    CHECK(run_cmd(cmd) == 0, "S47 Fil-C compiles emitted C with 0 warnings");
    CHECK(run_cmd("/tmp/stigi-c8-bin-filc") == 0,
          "S47 Fil-C compiled binary exits 0 (zero memory/capability panics)");
    (void)unlink("/tmp/stigi-c8-bin-filc");
  }

  int32_t test_idx = stigi_image_find(&door.image, "Demo.Tests.checkAll",
                                      strlen("Demo.Tests.checkAll"));
  CHECK(test_idx >= 0, "find Demo.Tests.checkAll");
  Arena ea;
  arena_init(&ea);
  StigiEvalResult er = stigi_eval_test(&door.image, (uint32_t)test_idx, STIGI_DEFAULT_FUEL, &ea);
  CHECK(er.status == STIGI_OK && er.value.kind == STIGI_VAL_TEST_OK,
        "evaluator agrees with compiled binary");
  arena_free(&ea);

  /* Clean up /tmp/stigi-c8-* */
  (void)unlink("/tmp/stigi-c8-out.c");
  (void)unlink("/tmp/stigi-c8-out.o");
  (void)unlink("/tmp/stigi-c8-out.su");
  (void)unlink("/tmp/stigi-c8-out-gcc.s");
  (void)unlink("/tmp/stigi-c8-out-clang.s");
  (void)unlink("/tmp/stigi-c8-rt-gcc.o");
  (void)unlink("/tmp/stigi-c8-rt-clang.o");
  (void)unlink("/tmp/stigi-c8-bin");
  (void)unlink("/tmp/stigi-c8-bin-clang");
  (void)unlink("/tmp/stigi-c8-guarded.c");
  (void)unlink("/tmp/stigi-c8-guarded-gcc.s");
  (void)unlink("/tmp/stigi-c8-guarded-clang.s");
  (void)unlink("/tmp/stigi-c8-mutant.c");
  (void)unlink("/tmp/stigi-c8-mutant-gcc.s");
  (void)unlink("/tmp/stigi-c8-mutant-clang.s");
  (void)unlink("/tmp/stigi-c8-empty.su");
  (void)unlink("/tmp/stigi-c8-over.su");
  (void)unlink("/tmp/stigi-c8-badbytes.su");

  free(c1);
  free(c2);
  stigi_door_free(&door);

  if (g_failures == 0) {
    printf("c8_test: ALL PASS (masks=%u tail_loops=%u lemmas_erased=%u pure_defs=%u par_sites=%u)\n",
           st1.spectre_masks, st1.tail_loops, st1.lemmas_erased,
           st1.pure_defs, st1.parallel_safe_sites);
    return 0;
  }
  fprintf(stderr, "c8_test: %d FAILURES\n", g_failures);
  return 1;
}
