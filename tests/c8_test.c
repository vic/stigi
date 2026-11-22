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
      "pub def Demo.allocAndRead {region: Stigi.Region} () <lessOrEqual(0, 1)>: U8 =\n"
      "  Array.tryFill{region}(64, 'Z') match\n"
      "    case Filled(buffer) => buffer(0)\n"
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
      "pub def Demo.extractTag(item: Demo.Tag) <lessOrEqual(0, 1)>: U64 =\n"
      "  item match\n"
      "    case Number(value) => value\n"
      "    case Mixed(_, _, _) => 1\n"
      "    case Empty => 0\n";

  const char *u_extract_tail =
      "// prompt: extract word from tail-packed enum using expression-position and integer match.\n"
      "pub def Demo.extractTail(packed: Demo.TailPack) <lessOrEqual(0, 1)>: U64 =\n"
      "  val chosen: U64 = packed match\n"
      "    case Wide(word, flag) =>\n"
      "      flag match\n"
      "        case 1 => word\n"
      "        case _ => 0\n"
      "    case EmptyPack => 0\n"
      "  chosen\n";

  const char *u_signed =
      "// prompt: signed negative literal comparison.\n"
      "pub def Demo.signCheck(value: I32) <lessOrEqual(0, 1)>: I32 =\n"
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
      "pub def Demo.applyWrap {file: Stigi.File, mut: Stigi.Mut} (wrap: Demo.BufferWrap) <lessOrEqual(0, 1)>: Unit =\n"
      "  wrap match\n"
      "    case Slice(data) => Demo.taintedWrite(4, data, 0, 1)\n"
      "    case Absent => ()\n";

  const char *u_tailsum_ok =
      "// prompt: tail-recursive countdown.\n"
      "pub def Demo.countDown(limit: U64) <lessOrEqual(0, limit)>: U64 =\n"
      "  if limit < 1 then 0 else Demo.countDown(limit - 1)\n";

  const char *u_void_tail =
      "// prompt: void tail-recursive loop (Defect 4).\n"
      "pub def Demo.voidCount(limit: U64) <lessOrEqual(0, limit)>: Unit =\n"
      "  if limit < 1 then () else Demo.voidCount(limit - 1)\n";

  const char *u_lemma =
      "// prompt: erased zero bound lemma.\n"
      "pub lemma def Demo.zeroBound(value: U64) <lessOrEqual(0, value)>: <lessOrEqual(0, value)> =\n"
      "  @<lessOrEqual(0, value)>\n";

  const char *u_test_ok =
      "// prompt: verify countDown and extractTag.\n"
      "val Demo.Tests.checkAll {Stigi.Test}: Stigi.Test.Result =\n"
      "  Stigi.Test.expect(Demo.countDown(5) + Demo.extractTag(Number(7)), 7, \"0 + 7 == 7\")\n";

  const char *u_main_ok =
      "// prompt: program entry exercising all lowered constructs.\n"
      "pub def main {file: Stigi.File, mut: Stigi.Mut, region: Stigi.Region} () <lessOrEqual(0, 1)>: I32 =\n"
      "  Demo.zeroBound(5)\n"
      "  Demo.voidCount(3)\n"
      "  Demo.applyWrap(Absent)\n"
      "  val first: U8 = Demo.byteAt(3, \"abc\", 0)\n"
      "  val second: U8 = Demo.taintedAt(3, \"abc\", 1)\n"
      "  val guarded: U8 = Demo.taintedGuarded(3, \"abc\", 2)\n"
      "  val filledByte: U8 = Demo.allocAndRead()\n"
      "  val dynamicByte: U8 = Demo.allocLoop(200, 64)\n"
      "  val done: U64 = Demo.countDown(5)\n"
      "  val tagVal: U64 = Demo.extractTag(Number(7))\n"
      "  val tailVal: U64 = Demo.extractTail(Wide(9, 1))\n"
      "  val negInput: I32 = -5\n"
      "  val negWant: I32 = -1\n"
      "  val signResult: I32 = Demo.signCheck(negInput)\n"
      "  val signedCompareTrue: Bool = (-5 < 0)\n"
      "  if done == 0 && tagVal == 7 && tailVal == 9 && first == 'a' && second == 'b' && guarded == 'c' && filledByte == 'Z' && dynamicByte == 'Y' && signResult == negWant && signedCompareTrue then 0 else 1\n";

  const char *all_srcs[22] = {
      u_file,            u_mut,         u_region,        u_enum,        u_buf_enum,
      u_tail_enum,       u_alloc_use,   u_alloc_dyn,     u_alloc_loop,  u_extract,
      u_extract_tail,    u_signed,      u_at,            u_tainted,     u_tainted_guarded,
      u_tainted_write,   u_apply_wrap,  u_tailsum_ok,    u_void_tail,   u_lemma,
      u_test_ok,         u_main_ok
  };
  size_t all_lens[22];
  for (int i = 0; i < 22; i++) all_lens[i] = strlen(all_srcs[i]);

  StigiDoorResult dr;
  stigi_door_result_init(&dr);
  CHECK(stigi_door_submit(&door, all_srcs, all_lens, 22, &dr, &d) == STIGI_OK, "door submit");
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
  CHECK(stigi_emit_c(&door, &c1, &l1, &st1, &d) == STIGI_OK, "emit 1");
  CHECK(stigi_emit_c(&door, &c2, &l2, &st2, &d) == STIGI_OK, "emit 2");
  CHECK(c1 != NULL && c2 != NULL && l1 == l2 && memcmp(c1, c2, l1) == 0,
        "determinism: two emissions are byte-equal");

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
  CHECK(st1.tail_loops >= 2, "tail-recursive defs (value + void) lowered to loops");
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
  Status su_st = stigi_emit_check_su("/tmp/stigi-c8-out.su", &door.image, su_err, sizeof su_err);
  if (su_st != STIGI_OK) {
    fprintf(stderr, "FAIL stigi_emit_check_su: %s\n", su_err);
  }
  CHECK(su_st == STIGI_OK, "-fstack-usage gate passes");

  /* Negative .su test 1: empty .su file must fail closed (Defect 12) */
  {
    FILE *ef = fopen("/tmp/stigi-c8-empty.su", "w");
    if (ef != NULL) (void)fclose(ef);
    CHECK(stigi_emit_check_su("/tmp/stigi-c8-empty.su", &door.image, su_err, sizeof su_err) != STIGI_OK,
          "Defect 12: empty .su file fails closed (0 matched)");
  }
  /* Negative .su test 2: strict bytes <= fb without +64 slack (Defect 13) */
  {
    FILE *bf = fopen("/tmp/stigi-c8-over.su", "w");
    if (bf != NULL) {
      fprintf(bf, "Demo/countDown.sc:1:1:Stigi_Demo_countDown\t16\tstatic\n");
      (void)fclose(bf);
    }
    CHECK(stigi_emit_check_su("/tmp/stigi-c8-over.su", &door.image, su_err, sizeof su_err) != STIGI_OK,
          "Defect 13: strict bytes > fb (16 > 8, no +64 slack) fails closed");
  }
  /* Negative .su test 3: invalid byte count fails strtoull endptr validation (Defect 13) */
  {
    FILE *mf = fopen("/tmp/stigi-c8-badbytes.su", "w");
    if (mf != NULL) {
      fprintf(mf, "Demo/countDown.sc:1:1:Stigi_Demo_countDown\t8xyz\tstatic\n");
      (void)fclose(mf);
    }
    CHECK(stigi_emit_check_su("/tmp/stigi-c8-badbytes.su", &door.image, su_err, sizeof su_err) != STIGI_OK,
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
