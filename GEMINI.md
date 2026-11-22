# GEMINI.md — Working on Stigi

Stigi is a Scala 3-like systems language compiled to portable ISO C11 that proves memory, bounds, arithmetic, capability-state, termination, and speculation safety at compile time with stratified Datalog, and compiles itself to a byte-identical fixpoint (`C2 == C3`).

This file is the operational guide for agents working in `/home/vic/hk/stigi`. **The spec and tenets are law**: always read `vic/AGENTS.md`, `vic/tenets/tenets.md` (T0–T20), `vic/tenets/rulings.md`, `vic/plan/lessons.md`, `vic/HANDOFF.md`, `vic/plan/impl-plan.md`, and the `vic/spec/*.md` files for your rung before acting. A tenet beats this file; a newer ruling beats a tenet.

---

## 1. Repository Layout

| Path | Purpose | VCS |
|---|---|---|
| `src/`, `tests/` | Ladder C hand-written C11 seed (`build/stigi-seed`), linked against `/home/vic/hk/tupu` (`libtupu.a`) | `jj` in `/home/vic/hk/stigi` |
| `tree/` | Stored snapshot of admitted L0 Stigi units (`X/Y/z.sc` + `index` + `cache`). **Written ONLY by the `store` frame — never edit directly.** | `jj` in `/home/vic/hk/stigi` |
| `.agents/plugins/stigi/` | Native Antigravity MCP plugin wiring `build/stigi-seed --mcp --rules vic/spec/datalog/all.dl` | `jj` in `/home/vic/hk/stigi` |
| `Justfile`, `flake.nix` | Seed build, GCC/Clang/ASan+UBSan tests, Fil-C 0.687 backstop (`just filc`), hermetic `nix build path:.` | `jj` in `/home/vic/hk/stigi` |
| `vic/` | Nested `jj` repo holding specs (`spec/`), Datalog oracle & D0/D1 suites (`spec/datalog/`), tenets/rulings (`tenets/`), plan/experiments/handoff (`plan/`, `HANDOFF.md`, `state.md`) | Separate `jj` in `/home/vic/hk/stigi/vic` |
| `/home/vic/hk/tupu` | Our in-process C11 Datalog engine (replaces Soufflé per S91) | Separate `jj` in `/home/vic/hk/tupu` |

**VCS rules (mandatory):**
- **ALWAYS use `jj`, NEVER `git`.**
- In `/home/vic/hk/stigi/vic`: **NEVER touch bookmark `claude`**. Commit spec/doc/experiment updates on bookmark `gemini` (or the active non-`claude` bookmark), and **never quote or name `vic` in `./vic` commit messages**.

---

## 2. How Stigi Code Is Written (T12 Frame-Only Methodology)

Stigi is image-based, not file-based. Shelling out and writing `.sc` files on disk is slow, burns tokens, and violates T12.

1. **NEVER create, read, edit, or write `.sc` files on disk, and NEVER shell out with heredocs to send frames.**
2. **ALL Stigi coding goes through the native `stigi_stigi` MCP server** (`stigi_frame` tool + `read_resource` on `stigi://summary/`, `stigi://laws/`, `stigi://tests`, `stigi://unit/<Unit.Name>`).
   - Alternative native socket transports built into `stigi-seed` (for harnesses without MCP): core Unix socket (`--serve <sock>`), `--cc-socks <sock> [--peer <name>]` (`msgV:1` NDJSON), or `--mcp --socket <sock>` bridge.
3. **Only `store /home/vic/hk/stigi/tree` writes `tree/`.** Never edit `tree/**/*.sc` by hand.

### Starting a Session
When a fresh session starts, the `stigi_stigi` MCP server starts with an empty in-memory image (and reads `vic/spec/datalog/all.dl` fresh). Hydrate it with one frame:

```text
load /home/vic/hk/stigi/tree
```

Then verify state via MCP:
- `stigi_frame`: `tests`
- `stigi_frame`: `laws`
- `read_resource`: `stigi://summary/`

### Core Frame Syntax (`stigi_frame`)
- **Admit new units (atomic transaction — all-or-nothing, S114 dependency-closed components):**
  ```text
  batch
  new // prompt: one-line statement of what the unit must do.
  pub enum Stigi.Example.Outcome:
    case Accepted(value: U64)
    case Rejected
  new // prompt: compute something proved safe.
  pub def Stigi.Example.compute(limit: U64) <below(limit, 1024)>: U64 =
    limit
  ```
  *Note: separate units inside `batch` with `new ` or `edit <Name> :=` — NEVER with `---`.*
- **Replace an existing unit:**
  ```text
  batch
  edit Stigi.Example.compute :=
  // prompt: updated unit text.
  pub def Stigi.Example.compute(limit: U64) <below(limit, 1024)>: U64 =
    limit
  ```
- **Dry-run a single unit without mutating the image:**
  ```text
  check
  // prompt: ...
  val Stigi.Tests.Example.probe {Stigi.Test}: Stigi.Test.Result =
    ...
  ```
- **Run all Tests & NegTests:** `tests`
- **List all admitted Laws (`lemma def`):** `laws`
- **Snapshot settled rung to disk for review/commit:** `store /home/vic/hk/stigi/tree`

---

## 3. Core Engineering & Verification Methodology

### A. Performance Law — Never Slower Than C
Stigi must match or beat hand-written C, never simulate native machine operations in slow loops:
- **Use native 1-cycle CPU primitives** (already wired in `src/{names,types,eval,extract,emit}.c`):
  - `U64.mix`, `U64.wrappingAdd`, `U64.wrappingSubtract`, `U64.wrappingMultiply`, `U64.bitwiseAnd`, `U64.bitwiseOr`, `U64.bitwiseXor`, `U64.shiftLeft`, `U64.shiftRight`
  - `U32.wrappingAdd`, `U32.wrappingSubtract`, `U32.wrappingMultiply`, `U32.rotateRight`, `U32.bitwiseAnd`, `U32.bitwiseOr`, `U32.bitwiseXor`, `U32.shiftLeft`, `U32.shiftRight`
  - Explicit casts: `U64.ofU8`, `U64.ofU32`, `U64.ofI32`, `U64.ofI64`, `U8.ofU64`, `U32.ofU64`, `I32.ofU64`, `I64.ofU64`
- **Data layout**: flat columnar Structure-of-Arrays (SoA) tables with `U64` indexes (`below(index, capacity)`) — zero recursive heap graphs, 64B cache-line alignment, forward linear scans friendly to L1 hardware prefetchers.
- **Zero steady-state page faults**: warm arena pools (`rss_delta=0 KB, minflt=0, majflt=0` on 1,000 real Datalog checks).
- **Embarrassingly parallel purity**: keep compiler passes pure or explicitly effect-scoped so independent units run in parallel without locks (T18).

### B. Soundness & Spec-First Law (T0, T6, T7, T14)
- **Never paper over a Datalog rejection** with bogus guards, unused branches, or weakened rules.
- If a rejection exposes a **real gap or bug in `vic/spec/datalog/*.dl` or `src/`**:
  1. **Stop the rung.**
  2. Fix the rule/spec first and record the ruling in `vic/tenets/rulings.md`.
  3. Add a D0 golden case + armed mutant in `vic/spec/datalog/cases/` and prove it on `tupu` (`run.sh` + `d1/run_d1.sh`).
  4. If the seed C code (`src/`) changed, run `just test` AND `just filc`, then atomically rebuild `build/stigi-seed` (see §5).
  5. Resume the rung.
- **No slop anywhere (`tenet-gate`)**: zero `abort`, `trap`, `exit`, `panic`, `unreachable`, `todo`, `fixme`, `skip`, `later`, `stub`, `placeholder`, `unsupported` in code, tests, comments, or prompts.

### C. Mandatory Gate for Every Settled Rung (S1–S9)
A rung is NOT done until every item passes in the same session:
1. **Spec review**: re-read the rung's `vic/spec/*.md` before designing.
2. **Real tests & proofs inside the image**:
   - POS Tests in a sibling namespace `Stigi.Tests.<Area>.*` (proving real behavior / official vectors and exercising every `pub` unit per T10 `pub-unused`).
   - NegTests (`{Stigi.NegTest}: Never`) proving exact `Stigi.Code.*` rejections.
   - At least one Law (`pub lemma def`) checked by `laws`.
3. **Door check**: `tests` and `laws` pass 100%, then `store /home/vic/hk/stigi/tree`.
4. **3-Compiler + Fil-C 0.687 Build & Run Gate** on the emitted C11:
   ```bash
   /home/vic/hk/stigi/build/stigi-seed \
     --rules /home/vic/hk/stigi/vic/spec/datalog/all.dl \
     --load /home/vic/hk/stigi/tree \
     --emit /tmp/stigi-rung.c

   # CRASH LINE: zero abort( or exit(
   ! rg -q '\b(abort|exit)\s*\(' /tmp/stigi-rung.c

   # 1. GCC + stack usage
   gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 \
     -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks \
     -fstack-usage /tmp/stigi-rung.c -o /tmp/stigi-rung-gcc && /tmp/stigi-rung-gcc

   # 2. Clang
   clang -std=c11 -Wall -Wextra -Werror -pedantic -O2 \
     -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks \
     /tmp/stigi-rung.c -o /tmp/stigi-rung-clang && /tmp/stigi-rung-clang

   # 3. Fil-C 0.687 (memory-safety backstop, S47 — zero panics)
   /nix/store/m6dqwwfjpxnmf1gmka33mkvacgrrpxql-filc-cc-wrapper-/bin/cc \
     -std=c11 -Wall -Wextra -Werror -pedantic -O2 \
     -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks \
     -DSTIGI_FILC_BUILD /tmp/stigi-rung.c -o /tmp/stigi-rung-filc && /tmp/stigi-rung-filc
   ```
5. **Clean commit**: commit the settled `tree/` snapshot (and any seed fixes) with `jj commit`.

---

## 4. Battle-Tested L0 & `all.dl` Idioms (Avoid Common Traps)

These exact rules are enforced by `tenet-gate`, `all.dl`, and `src/emit.c`:

- **Tenet-gate naming**:
  - Every unit starts at column 0 with `// prompt: <non-empty text>`.
  - No trailing digits on any name (`Stigi.Hash`, NOT `Stigi.Blake3`; `itemOne`, NOT `item1`).
  - No 1–2 letter camelCase/PascalCase subwords outside `{of, at, to, in, is, by, or, on, if, as}` (`Ok`, `Id`, `Eq`, `Op`, `wZero` are rejected as `cryptic-name`).
  - Keep internal helpers non-`pub`; put tests that exercise `pub` units in a sibling namespace (`Stigi.Tests.<Area>.*`) to satisfy T10 `pub-unused`.
- **Entry facts on every `def` (T4 `facts-missing`)**:
  - Every `def` must carry a `<...>` fact group.
  - `bounds.dl` has `AX_U64ZERO` (`0 <= x`) for `U64` only, NOT `U8`/`U32`/`I64`/enums. For pure helpers taking `U8`/`U32`/`I64`/enums, use `<lessOrEqual(0, 1)>` (universally provable at every call site) rather than `<lessOrEqual(0, byte)>`.
- **Narrowing casts**:
  - `U8.ofU64(x)` requires `<below(x, 256)>` -> compute `U8.ofU64(U64.bitwiseAnd(x, 255))` (proved via `andOp` + `TRANSLB` in 1 CPU cycle).
  - `U32.ofU64(x)` requires `<below(x, 0x100000000)>` -> compute `U32.ofU64(U64.bitwiseAnd(x, 0xffffffff))`.
- **Allocations (`Array.tryFill`)**:
  - Always match `Array.tryFill{region}(len, typedZero) match case FillFailed => ... case Filled(arr) => ...` directly — do NOT bind its enum result to a `val` before matching.
  - Always pass a typed fill variable (`val zeroByte: U8 = 0`, `val zeroWord32: U32 = 0`, `val zeroWord: U64 = 0`) so the emitter infers the exact C element pointer type (`uint8_t *`, `uint32_t *`, `uint64_t *`).
- **Syntax & Emitter constraints**:
  - Put `&&` / `||` expressions on one line (no trailing `&&`/`||` before newline); bind intermediate `val`s for readability.
  - Patterns are 1 level deep (`nested-pattern`), exhaustive over enums with no `_` arm. Single-case enums support `val Ctor(a, b) = value`.
  - Never write a self-assignment (`x = x`) — Clang `-Werror,-Wself-assign` rejects it. Use `()` in empty `else` branches or `x = if cond then nextVal else x`.
  - No decided conditions (`branch-decided`): never test a condition already implied by entry facts or loop invariants, and avoid `while remaining > 0` initialized to a constant literal without a parametric bound.
- **Termination (`decreases`)**:
  - Every `while` loop starts with `@<..., decreases(measure)>`.
  - On early-exit loops (`&& !found`), advance the loop index on **both** branches of `if`, or bound with a strictly incrementing `stepIndex` (`decreases(limit - stepIndex)`).
  - For chunked stepping (`+ 64`, `+ 1024`), compute `val plusStep: U64 = cursor + 64; val nextCursor: U64 = if plusStep < end then plusStep else end` and track a monotonic `stepIndex` (`+ 1` per turn) for `decreases`.

---

## 5. Build, Test, and Seed Hygiene Commands

- **Full seed suite (GCC + Clang + ASan+UBSan + S57 spec-budget):**
  ```bash
  just test
  ```
- **Fil-C 0.687 seed backstop (compiles `libtupu-filc.a` + `stigi-seed-filc` + all 6 test suites + emitted C11):**
  ```bash
  just filc
  ```
- **Hermetic Nix build & check:**
  ```bash
  nix build path:.
  ```
- **Atomic seed rebuild while `stigi_stigi` MCP is running (prevents `ETXTBSY`):**
  ```bash
  gcc -std=c11 -Wall -Wextra -Werror -pedantic -O2 \
    -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks \
    -Isrc -I/home/vic/hk/tupu/result/include \
    src/*.c /home/vic/hk/tupu/result/lib/libtupu.a -pthread \
    -o build/stigi-seed.new && mv build/stigi-seed.new build/stigi-seed
  ```
  *Note: the already-running `stigi_stigi` MCP server keeps the old binary and `all.dl` in memory until restarted; running `build/stigi-seed --load tree --emit ...` on disk immediately tests the rebuilt binary and updated `all.dl`.*
- **D0 & D1 Datalog oracle suites on `tupu`:**
  ```bash
  cd vic/spec/datalog
  SC=$(find /home/vic/hk/tupu -maxdepth 3 -name 'souffle-compat*' -executable | head -n 1)
  SOUFFLE="$SC" bash cases/run.sh
  bash d1/run_d1.sh
  ```

---

## 6. Development Ladder (`vic/plan/impl-plan.md`)

| Ladder | Scope | Gate |
|---|---|---|
| **D (D0–D2)** | Datalog rule families (`vic/spec/datalog/*.dl`), 788+ golden cases & mutants on `tupu`, 3 D1 L0 compiler units | 0 diff across all 15 families, 0 rejects on D1, all armed controls reject |
| **C (C0–C9)** | Hand-written C11 seed (`src/`, `tests/`), native 1-cycle primitives, UDS / `--cc-socks` / `--mcp` transports | `just test` (GCC+Clang+ASan/UBSan), `just filc` (Fil-C 0.687), `nix build path:.`, `rss_delta=0 KB` / `minflt=0` |
| **S (S1–S9)** | Self-hosted L0 compiler entered exclusively via MCP frames: S1 Datalog, S2 Lex/TenetGate/Parse/Print, S3 Hash(BLAKE3)/Names/Identity, S4 Types/Eval, S5 Extract, S6 Door, S7 Emit, S8 Arena/Heap/Posix/Server, S9 Store/`main` | Per rung: Tests + NegTests + Laws + `store tree` + 0 `abort`/`exit` + GCC (`-fstack-usage`) + Clang + Fil-C 0.687 |
| **B (B1–B4)** | `stigi.1 = cc(C1)` -> `stigi.1` loads `tree/` (`seed == stigi.1 == tupu`) -> `stigi.2 = cc(C2)` -> `cmp C2 C3` + armed 1-byte mutant -> perf bars (`vic/spec/performance.md` §3) -> retire seed | Byte-identical fixpoint, 3-way verdict parity, Fil-C + `.su` + `perf stat` bars met |
| **G** | Growth rungs (C ABI export, `Par`/`Pool`/disentangled concurrency, FP²/FIP trees, Lean certs) | One feature per rung, rules in `tupu` first, full B2+B3 |
