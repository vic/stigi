# Stigi

[![Sponsor](https://img.shields.io/badge/sponsor-vic-ea4aaa?logo=githubsponsors)](https://github.com/sponsors/vic)


> Stigi is very much a work in progress! [./src](./src) contains our seed compiler and [./tree](./tree) the current bootstrapped Stigi source code.
>
> Stigi is an ambitious project, that is the very
> reason I'm dedicating my own time and resources to it, because I absolutely want to make it possible.
>
> And that is also why it needs your support, from leaving an star, showing it to your friends,
> [trying it or sponsoring its development](#11-sponsor-early-adopters-and-license-agpl).

**A systems language and live verification door built for the era when AI writes most of the code -- and humans still have to trust, run, and understand it.**

Stigi looks like clean Scala 3 on the surface and compiles to portable, zero-check C11 (or WebAssembly components and GPU kernels) with no garbage collector, no runtime panics, no out-of-memory kills, and no ambient authority. Beneath that surface sits a single unified Datalog reasoning engine that proves memory safety, bounds, integer arithmetic, stack limits, stateful resource protocols, time and space complexity, and permission manifests **before a single line of code is allowed into the program image**.

We did not build Stigi to be another systems language that fights Rust or Zig on syntax preferences. We built Stigi because AI broke the old software contract:

- **AI writes code faster than any team can read it**, and silently cheats around weak checks -- deleting tests, mocking away failures, weakening assertions, or stuffing codebases with dead glue.
- **Today's AI runtimes pay a massive tax for blind execution**: containers, microVMs, over-provisioned memory limits, sudden OOM kills, fuel traps that leave files and sockets half-open, and partial traces that break on replay.
- **Tokens are literally money**, yet existing languages, file trees, shell greps, and verbose compiler logs burn most of an agent's budget just discovering context and retrying errors.
- **Tests have become a blind substitute for understanding.** Teams now run untrusted AI code in CI just to observe surface effects -- the same crisis mathematics hit when AI began producing multi-million-line formal proofs that machines accept and zero humans can comprehend.

Stigi answers all four problems from one core design: **the durable artifact is a content-addressed, mathematically checked program image -- never loose text files on disk.** From that single image, Stigi projects a token-efficient protocol for AI agents, a compact abstract mathematical dossier for human reviewers, and a proved runtime envelope for production infrastructure.

---

## Quick CLI & Agent Session Reference

```bash
# Load a reviewed source tree through the door from an empty image
stigi load ./tree

# Inspect the human-level Proved Theory Sheet of any root or namespace
stigi abstract App.Main

# Review semantic changes (authority, protocols, laws, flow, envelopes)
stigi diff --from prev --to head

# Query the live image by full contract (types + facts + capabilities + O(x) cost)
stigi search "(count: U64, bytes: Array[U8](count)) -> ? {pure} <cost <= O(count)>"

# Pull a 1-turn editing context slice for an agent
stigi slice App.Parser.parseHeader --depth 1

# Emit verified portable C11 or a C ABI library with generated header
stigi emit ./out/app.c
stigi emit --export-c ./out/libtokenizer
```

---

## Table of Contents

1. [Why Stigi Exists](#1-why-stigi-exists)
2. [A Two-Minute Tour in Code](#2-a-two-minute-tour-in-code)
3. [One Reasoning Engine Instead of Five Fragile Layers](#3-one-reasoning-engine-instead-of-five-fragile-layers)
   - [Memory Without a Garbage Collector or Lifetime Annotations](#memory-without-a-garbage-collector-or-lifetime-annotations)
   - [Zero Panics, Zero Silent Overflow, Zero Stack Explosions](#zero-panics-zero-silent-overflow-zero-stack-explosions)
   - [Zero Ambient Authority and Linear Resource Protocols](#zero-ambient-authority-and-linear-resource-protocols)
   - [Automatic AI-Slop Elimination (Tree Shaking and Compile-Time Folding)](#automatic-ai-slop-elimination-tree-shaking-and-compile-time-folding)
4. [Checked Complexity (`O(x)`), Full-Telescope Search, and Verified Synthesis](#4-checked-complexity-ox-full-telescope-search-and-verified-synthesis)
   - [Proving Concrete Cost and `O(...)` Bounds Without SMT Timeouts](#proving-concrete-cost-and-o-bounds-without-smt-timeouts)
   - [Amortized Bounds Over State Records](#amortized-bounds-over-state-records)
   - [Searching the Image by the Whole Contract](#searching-the-image-by-the-whole-contract)
   - [Verified Code Synthesis (No Opaque Macros)](#verified-code-synthesis-no-opaque-macros)
5. [Built to Beat Cheating AI Agents: The Law Loop](#5-built-to-beat-cheating-ai-agents-the-law-loop)
   - [How Agents Cheat -- and How the Door Stops Every Trick](#how-agents-cheat----and-how-the-door-stops-every-trick)
   - [Humans Own the Laws; Agents Own the Implementations](#humans-own-the-laws-agents-own-the-implementations)
6. [The Proved Runtime Envelope (What AI Infrastructure Overlooked)](#6-the-proved-runtime-envelope-what-ai-infrastructure-overlooked)
   - [Four Runtime Layers Replaced by One Admission Verdict](#four-runtime-layers-replaced-by-one-admission-verdict)
   - [Two-Mode Scheduling: Exact Packing and Safe Overcommit](#two-mode-scheduling-exact-packing-and-safe-overcommit)
   - [Zero-Hypervisor Deterministic Replay and Multiverse Forking](#zero-hypervisor-deterministic-replay-and-multiverse-forking)
   - [How Stigi Compares to Existing Runtimes](#how-stigi-compares-to-existing-runtimes)
7. [Tokenomics First: Built for AI-Authored Software](#7-tokenomics-first-built-for-ai-authored-software)
   - [Why We Compress Reads and the Protocol -- Not Write Syntax](#why-we-compress-reads-and-the-protocol----not-write-syntax)
   - [How an Agent Session Actually Works in Stigi](#how-an-agent-session-actually-works-in-stigi)
8. [Understanding What Code Does Without Reading Slop: The Proved Theory Sheet](#8-understanding-what-code-does-without-reading-slop-the-proved-theory-sheet)
   - [Why Giant Visual Graphs Fail -- and What Humans Are Actually Good At](#why-giant-visual-graphs-fail----and-what-humans-are-actually-good-at)
   - [Inside a Proved Theory Sheet](#inside-a-proved-theory-sheet)
   - [Reviewing Theory Diffs Instead of 800-Line Text Diffs](#reviewing-theory-diffs-instead-of-800-line-text-diffs)
   - [Why Testing in Stigi Is Not "Running Untrusted Code"](#why-testing-in-stigi-is-not-running-untrusted-code)
9. [Parallelism, Native C Interop, Embedded Boards, WebAssembly, and GPUs](#9-parallelism-native-c-interop-embedded-boards-webassembly-and-gpus)
10. [Self-Hosting, Independent Certification, and Exact Limits](#10-self-hosting-independent-certification-and-exact-limits)
11. [Sponsor, Early Adopters, and License (AGPL)](#11-sponsor-early-adopters-and-license-agpl)

---

## 1. Why Stigi Exists

I have no problem with AI writing code. The problem is living with AI-generated slop that nobody understands, nobody can safely run without a wall of containers around it, and nobody can prove actually obeys its specification.

Look at how software works right now when AI agents do the coding:

1. **You pay twice for safety, and still get leaks.** The agent generates code in Python, TypeScript, Go, or Rust. You wrap the result in Docker or WebAssembly, add generous RAM headroom because you have no idea what peak memory will be, set arbitrary timeouts, and pray that an OOM kill doesn't drop a database lock or half-written file on the floor.
2. **You burn most of your token budget fighting files and compiler noise.** Agents grep across directories, read entire 1,000-line files to check one function signature, rewrite whole modules to patch three lines, and burn turn after turn re-reading giant error logs.
3. **You review by running tests, not by understanding.** Nobody reads 600 lines of agent-authored boilerplate anymore. Teams hit green in CI and merge -- even when the agent quietly mocked a handler, weakened a check, or introduced subtle resource or security regressions.
4. **Even formal proofs got unreadable.** When an AI writes millions of lines of verified code or proof scripts, a machine can check every step, yet human engineers lose the mental model of the system completely.

Stigi flips the architecture upside down. Every unit of code enters a **single live image** through **one strict admission door**. Before anything is admitted:

- One deterministic Datalog engine checks memory ownership, array bounds, arithmetic ranges, stack depth, resource protocols, permissions, complexity, and universally quantified laws together.
- Unused code, unused variables, dead branches, idle facts, and constant pure computations are shaken out automatically so slop never enters the image.
- Rejected edits come back with the exact missing fact, a concrete counterexample when a property is false, and a pre-verified fix that an agent can accept in two tokens.
- Humans govern the system at the level of **theorems, protocols, permission manifests, decision tables, and checked explanations** -- not raw walls of implementation text.

---

## 2. A Two-Minute Tour in Code

Stigi borrows the clean, indentation-based surface of Scala 3 so any human who reads Python, Kotlin, Scala, or Rust can read a Stigi unit immediately. Every unit starts with a plain one-line `// prompt:` stating what it is for, uses full readable words (never cryptic two-letter abbreviations), and carries its compile-time guarantees right inside `<...>` angle brackets.

Here is a complete function that counts matching bytes in a buffer:

```scala
// prompt: count how many bytes in a block match a target byte.
pub def Bytes.countMatches(size: U64, block: Array[U8](size), target: U8): U64 <lessOrEqual(result, size)> =
  var matches: U64 = 0
  var index: U64 = 0
  while index < size do
    @<lessOrEqual(matches, index), lessOrEqual(index, size)>
    matches = if block(index) == target then matches + 1 else matches
    index = index + 1
  matches
```

Look closely at what is **not** in that function, and what Stigi **already proved** at admission:

- **No runtime bounds check on `block(index)`**: the loop guard gives `below(index, size)`, and `size` is part of `block`'s type. The emitted C reads `block[index]` directly with zero branch overhead.
- **No integer overflow check on `matches + 1` or `index + 1`**: both are strictly below `size`, which already fits in `U64`.
- **No runtime assertion on the return value**: callers receive `<lessOrEqual(result, size)>` as a proved fact they can immediately use to index another buffer of length `size` (or prove loop progress) without adding defensive `if` statements.
- **No redundant runtime guards**: if someone writes `if index < size then block(index) else 0` inside that loop, Stigi rejects it as a decided branch -- facts replace runtime `if`s.

Now look at how Stigi handles allocation, errors, and stateful resources with zero exceptions and zero `null`:

```scala
// prompt: read a bounded header from an open file into a fresh buffer, then close the file.
pub def Storage.readHeader {region: Stigi.Region, file: Stigi.File} <at(file, Open)> (limit: U64)
    <lessOrEqual(limit, 4096)>: Storage.HeaderResult <at(file, Closed)> =
  val Filled(buffer) = Array.tryFill{region}(limit, 0) else case FillFailed =>
    val _ = file.close()
    OutOfMemory
  val outcome: Stigi.File.Read = file.read(limit, buffer)
  val _ = file.close()
  outcome match
    case BytesRead(count) => HeaderLoaded(count, buffer)
    case ReadFailed(code) => StorageError(code)
```

Notice three things that happen automatically here:

1. **Out-of-memory is just data (`FillFailed`), never a crash.** Notice what happens if you forget `file.close()` on the `FillFailed` branch -- the door refuses to admit the function because `file` did not reach its required `Closed` stop state on every path.
2. **You never write `free()` and you never write lifetime annotations.** When `StorageError` returns without `buffer`, Stigi knows `buffer`'s last use was `file.read(...)` and inserts its free automatically before returning. When `HeaderLoaded` returns `buffer`, ownership moves cleanly to the caller inside the same region.
3. **Every failure arm must be exercised.** Stigi's fault-injecting test runner forces `FillFailed` and `ReadFailed` deterministically at compile time; an untested failure handler is rejected at admission.

And if an agent submits a buggy version -- say, indexing `table(index)` without proving `index < count` -- nothing runs and nothing corrupts the image. The door replies directly with the exact missing fact and a checked repair:

```text
rejected {Demo.Table.peek}: index-unproved at Demo.Table.peek.body[0]
  need: below(index, count)
  fix1 (checked): add entry fact <below(index, count)>
  lesson: index-unproved (see cached prefix)
```

The agent replies `accept fix1` -- two tokens -- and the unit enters the image cleanly.

---

## 3. One Reasoning Engine Instead of Five Fragile Layers

In most ecosystems, safety is scattered across separate tools that do not talk to each other: a type checker here, a borrow checker there, an external linter, a static analyzer that produces false warnings nobody reads, a runtime sanitizer, and a container sandbox. Because each tool sees only a slice of the program, optimizations stay conservative and gaps open between layers.

In Stigi, **memory, data layout, purity, bounds, arithmetic, resource protocols, permissions, complexity, and dead-code elimination are all facts inside one unified Datalog engine.** Every pass sees everything proved by every other pass.

### Memory Without a Garbage Collector or Lifetime Annotations

Writing fast systems code normally forces an ugly choice: accept garbage-collector pauses and unpredictable memory spikes (Go, Java, JVM Scala), fight manual `free` bugs (C, C++, Zig), or spend half your effort wrestling borrow-checker syntax and reference-counting wrappers (Rust).

Stigi takes a third path:

- **Inferred single ownership**: Every array, record, or cell has exactly one owner at every program point, inferred completely by the door. No `&'a mut` sigils, no `Rc`/`Arc` noise.
- **Freed at last use, not at scope exit**: Stigi computes the exact last use of every object across control flow and frees or reuses its slot immediately -- even promoting non-escaping objects directly onto aligned stack frames.
- **Flat memory enforced by the door**: Holding a dead object alive while continuing to allocate is a compile-time rejection, not a memory leak you discover at 3 a.m. in production. Steady-state server workloads run with zero major and zero minor page faults.
- **Safe in-place growth and linear trees**: Growing an array moves ownership of the old buffer cleanly (`tryGrow`) so stale aliases can never survive; recursive data structures live either in flat, cache-aligned node tables with proved `<below(child, count)>` indexes or as constant-stack boxed linear trees updated in place.

### Zero Panics, Zero Silent Overflow, Zero Stack Explosions

Standard libraries in almost every modern language hide panics (`unwrap`, bounds traps, slice panics, stack overflows, allocation aborts). Stigi bans them across the entire program -- standard library included:

- Every integer `+`, `-`, `*`, `/`, `%`, and bit shift must be proved inside its type's exact range (or explicitly use named wrapping operations, which are flagged in review diffs).
- Every call cycle proves termination (`decreases`) or a bounded recursion depth; tail recursion lowers to flat loops with constant stack usage.
- Every root function has a proved worst-case stack budget summing all Stigi frames, the declared stack cost of any external C call on the path, and the OS signal reserve -- cross-checked against compiler stack-usage tables and guarded stacks.
- The emitted C contains zero `abort(` calls and zero `exit(` calls.

### Zero Ambient Authority and Linear Resource Protocols

Supply-chain attacks and rogue AI tool calls happen because almost every language hands ambient authority to every imported package: any helper can read environment variables, open sockets, touch the filesystem, or execute shell scripts at build time.

In Stigi:

- **No ambient I/O anywhere -- runtime or build time.** There are no build scripts, no hidden global state, and no unchecked system calls. A unit can only perform an effect if its caller explicitly hands it a capability in `{...}`.
- **Narrowed capabilities become proved permission manifests.** Capabilities can be derived and narrowed -- for example, `Files.under("./data")` or `Socket.to("api.example.com")`. Asking the image for `manifest` prints a mathematically proved statement of everything a root can touch:

```text
$ stigi manifest Billing.Service.main
root Billing.Service.main:
  files:   read-only under ./config
  network: outbound TLS to payments.internal:443
  clock:   monotonic only
  shell:   none
  externs: 2 counted C bindings (see `trust`)
```

- **Stateful protocols and cross-resource denials.** Capabilities declare finite state machines (`Open -> Closed`, `Prepared -> Finished`, `Submitted -> Completed | Cancelled`). Stigi proves operations are only called in legal states, branches agree on states at joins, handles reach a stop state on every exit path, and two aliases to the same stateful handle are never passed to one call. You can even declare cross-resource ordering rules directly:

```scala
// prompt: count rows while proving the statement finishes before the database closes.
pub def Billing.countRows {database: Billing.Database} <at(database, Open)>
    (size: U64, queryText: Array[U8](size)): U64 <at(database, Closed)> =
  handle statement = Billing.Statement(database, size, queryText)
  @<not (close(database), at(statement, Prepared))>
  val rows = statement.stepCount()
  statement.finish()
  database.close()
  rows
```

If anyone later reorders `database.close()` ahead of `statement.finish()`, the door rejects the edit immediately.

### Automatic AI-Slop Elimination (Tree Shaking and Compile-Time Folding)

AI agents love generating extra helper functions, unused intermediate variables, defensive branches that facts already prove, and constant expressions recomputed at runtime.

Every time a candidate batch passes checks, Stigi's door runs **whole-image tree shaking and compile-time partial evaluation** over the dependency graph:

- Unreachable functions, unused variables, unused expression results, unused parameters, and idle facts that change no proof are pruned before commit.
- Pure constant expressions and constant pure helper calls are evaluated at compile time and folded down to their values.
- The door re-verifies the shaken form and returns both what it removed (`shaken:`) and the minimal checked source (`minimal:`). Only the smallest sound program survives in the image.

---

## 4. Checked Complexity (`O(x)`), Full-Telescope Search, and Verified Synthesis

Type signatures tell you what shape of data goes in and comes out. They tell you nothing about whether a function runs in 10 steps or crashes your latency SLA with an accidental quadratic loop -- and they make finding or synthesizing the right function painful.

Stigi turns **performance guarantees** into first-class facts inside the same door.

### Proving Concrete Cost and `O(...)` Bounds Without SMT Timeouts

Pure Big-`O` notation by itself is easy for an AI agent to abuse: a loop running `1,000,000,000` fixed iterations is technically `O(1)`. So Stigi proves **exact 128-bit concrete step and allocation coefficients** first (exposing hidden constants immediately) and projects leading terms cleanly to asymptotic bounds like `O(1)`, `O(log n)`, `O(n)`, `O(n log n)`, and `O(n^2)`.

Because cost tracking lives inside the exact same stratified Datalog engine as bounds and termination -- with zero SMT solvers and zero LP solvers in the trusted checker -- verification stays fast, polynomial, and deterministic:

```scala
// prompt: find the index of a target byte in a sorted block in logarithmic time.
pub def Search.binaryFind(size: U64, block: Array[U8](size), target: U8)
    <lessOrEqual(1, size), cost(O(log size))>: Search.Lookup =
  Search.binaryStep(size, block, target, 0, size)
```

- Halving loops and balanced divide-and-conquer recurrences clear `O(log n)` and `O(n log n)` automatically; an uneven split that degrades to quadratic rejects `O(n log n)` and reports `O(n^2)`.
- Hidden work cannot be smuggled past the checker: allocations (`tryFill`), array copies, and external C calls all carry mandatory cost rows.

### Amortized Bounds Over State Records

Data structures like doubling buffers occasionally pay an `O(n)` resize while averaging `O(1)` per push. In Stigi, a state record declares a single linear potential function (always proved non-negative), and the door checks that every transition's step cost plus potential change stays within the amortized bound:

```scala
// prompt: append one byte to a doubling buffer in amortized constant time.
pub def Buffer.pushByte {region: Stigi.Region, Stigi.Mut}
    (state: Buffer.Growing, byteValue: U8) <amortized(O(1))>: Buffer.PushOutcome =
  ...
```

A doubling growth strategy clears `amortized(O(1))`. Change it to linear `+ 1` growth and the door rejects the contract immediately.

### Searching the Image by the Whole Contract

Instead of grepping function names across hundreds of files, humans and AI agents query Stigi's live image by the **entire telescope** -- parameter types (up to argument reordering), preconditions, postconditions, allowed capabilities, and proved complexity -- using `?` logic variables:

```text
$ stigi search (size: U64, block: Array[U8](size), target: U8) -> ? {pure} <cost <= O(log size)>
1. Search.binaryFind : (size: U64, block: Array[U8](size), target: U8) <lessOrEqual(1, size), cost(O(log size))> -> Search.Lookup
   proved bound: 9 * log2(size) + 4 steps, 0 heap allocs, 48 B stack
2. Search.lowerBound : (size: U64, block: Array[U8](size), target: U8) <cost(O(log size))> -> U64 <lessOrEqual(result, size)>
   proved bound: 8 * log2(size) + 3 steps, 0 heap allocs, 48 B stack
```

Search reads **proved** derived costs, never unverified comments or hand-written annotations -- so no agent can fake its way into a query result.

### Verified Code Synthesis (No Opaque Macros)

When an author or agent leaves an ephemeral typed hole `?(goal)` during a dry-run `check`, Stigi inspects the expected type, every fact in scope, the active capabilities, and the remaining complexity budget. Its bounded synthesizer filters out every candidate whose proved cost exceeds the hole's budget, searches for evidence chains, lemma invocations, exhaustive `match` skeletons, or short call compositions, and passes every candidate through the door before showing it:

```text
$ stigi synth Demo.Router.dispatch ?(handler) --budget O(log count)
candidate1 (checked, cost 11 * log2(count) + 6):
  Search.binaryFind(count, routes, key) match
    case FoundAt(slot) => DispatchHit(slot)
    case Missing       => DispatchMiss
```

Notice what Stigi refuses to do: **no hidden compile-time macro expansion.** Synthesized code enters the image only when explicitly accepted as plain, readable Stigi source that humans and tools can inspect, diff, and version in git.

---

## 5. Built to Beat Cheating AI Agents: The Law Loop

Anyone who has run coding agents at scale has watched them cheat. When a test fails or a proof gets hard, agents do what reward signals encourage: they delete the failing test, loosen the assertion, rewrite the test oracle to match their buggy code, assert `expect(true, true)`, mock a handler to ignore errors, unwrap a safe domain type into a raw integer, or invent a bogus helper fact that always evaluates to true.

Traditional languages cannot stop this because tests, specifications, and implementation code are just writable text files sitting in the same folder.

Stigi turns verification into a closed governance cycle we call **The Law Loop**:

```text
Human (reviewed git/jj tree)  --> owns laws, reference specs, public contracts, custom rules
AI Agent (live image frames)  --> owns implementations, proof bodies, optimizations, candidate laws
Stigi Door (Datalog checker)  --> proves everything, blocks weakening, shakes slop, audits trust
```

### How Agents Cheat -- and How the Door Stops Every Trick

| How AI Agents Cheat Elsewhere | What Happens in Stigi |
|---|---|
| **Delete or weaken failing tests and contracts** | Live agent sessions cannot drop or weaken any law (`lemma`), `Test`, `NegTest`, public signature/capability row, or custom rule. The door rejects the batch with `spec-weakened`. Only a human-reviewed tree load can change specifications. |
| **Edit the test oracle to match buggy code** | Any helper reachable only from tests and laws is frozen against live agent edits (and agents cannot add a dummy public call to unfreeze it). |
| **Forge custom facts to fake a proof** | Safety rules never read user-defined relations. Custom rules live strictly inside their own namespace, enter only via reviewed tree loads, appear in the `trust` audit, and are barred from ever being reached by any mathematical law. |
| **Write tautological or mocked-out tests** | Pre-fold checks reject `expect(true, true)` and `expect(f(1), f(1))`. Fault-injection rules require a real dataflow path from each failure arm's result into the test's expectation, so touching an error arm and ignoring it fails admission. |
| **Launder negative tests** | A negative test referencing a target unit must fail on a call to that unit, not on an unrelated dummy helper. |
| **Strip domain wrappers (`.as_str()` laundering)** | Nominal types and enums carrying facts can only be constructed inside their declaring namespace; outsiders can match and read them, never forge them. |
| **Hide silent wraparound or fake instances** | Review diffs explicitly call out every wrapping math operation, every new trait instance, every paired helper-plus-test addition, and any test exercising non-production handlers. |
| **Exploit a compiler or checker bug** | Continuous adversarial red-team suites attack the door against memory-safe Fil-C, sanitizers, exact 128-bit arithmetic differentials, hardware stack checks, and mandatory hostile negative tests for every single kernel unit and C binding. |

### Humans Own the Laws; Agents Own the Implementations

In Stigi, a **law** (`lemma def`) is a universally quantified theorem checked for *all* inputs -- not three hand-picked unit test examples:

```scala
// prompt: narrowing a proved offset inside a slice stays strictly inside the outer buffer.
pub lemma def Slice.offsetInsideOuter(outerLength: U64, sliceStart: U64, sliceLength: U64, offset: U64)
    <lessOrEqual(sliceStart + sliceLength, outerLength), below(offset, sliceLength)>:
    <below(sliceStart + offset, outerLength)> =
  ()
```

Laws are erased completely at code generation (zero runtime cost), re-checked automatically whenever any dependency changes, and audited to split every dependency into **kernel logic vs user definitions vs trusted assumptions**. An agent can rewrite a 400-line implementation from scratch to make it twice as fast -- and the door proves it still obeys every human-owned law, or refuses it naming the exact law broken.

For complex data transformations where you want full functional assurance beyond arithmetic and protocols (like sorting or compression), humans write a tiny 20-line pure reference model (`Spec.decode`), the agent writes the fast implementation (`Fast.decode`), and Stigi automatically runs seeded boundary property tests pairing the two -- erasing `Spec.decode` completely from the compiled binary while explicitly reporting that equivalence as **`TESTED`** in the law sheet.

---

## 6. The Proved Runtime Envelope (What AI Infrastructure Overlooked)

If you build or invest in AI agent platforms, tool runtimes, serverless workers, or RL rollout fleets, here is the core architectural advantage every other stack misses.

### Four Runtime Layers Replaced by One Admission Verdict

Every AI runtime today treats agent-written code as opaque when it arrives, so it wraps execution in four separate runtime mechanisms:

1. **Runtime sandboxes** (Docker, Firecracker microVMs, Wasm traps, OS seccomp/cgroups),
2. **Static memory headroom and timeouts** (over-provisioned container caps + sudden OOM kills),
3. **Partial traces** (best-effort logs and honor-system workflow replay that explodes with nondeterminism errors if code touches a clock, RNG, or file), and
4. **Manual permission review** (humans reading dependencies and scripts to guess what code might touch).

Worse, those layers fight each other. When a container OOM-kills or a WebAssembly engine traps out of fuel halfway through a tool call, your locks, files, sockets, and upstream APIs are left in torn, half-finished states.

In Stigi, **one admission verdict produces a single Proved Runtime Envelope** per unit unifying all four properties:

1. **Proved Narrowed Authority**: Exact capability manifest (directories, hosts, clocks, externs) -- enforced at admission and double-enforced when exported as WebAssembly components (the capability row equals the Wasm import list).
2. **No Stigi-Caused Death Mid-Effect**: Because failures and allocation shortfalls are typed values and every resource protocol must reach a declared stop state on every path (with every failure arm fault-tested), Stigi code never traps or panics mid-effect. Wherever the capability protocol models commit and rollback (for instance, write-temp-then-rename with stop states `Committed | RolledBack`), external state stays consistent even under memory exhaustion or I/O failure.
3. **Two-Mode Parametric Resource Envelope**: Exact proved stack bytes, concrete step bounds, and parametric peak heap bounds `heap(n)` fed directly to your scheduler.
4. **Complete-by-Construction Effect Journal**: Every external call -- including every LLM call -- passes through a typed capability, giving hypervisor-free deterministic replay and branching.

### Two-Mode Scheduling: Exact Packing and Safe Overcommit

A common objection to static resource bounds in AI runtimes is worst-case pessimism: an agent loop might have a `max_steps = 50` cap while averaging 3 steps, or a `max_tokens = 8192` ceiling while averaging 400 tokens. Packing every tenant for the worst case wastes 10x--50x DRAM.

Stigi's runtime envelope handles both workload shapes cleanly:

- **Mode 1 -- Exact Arrival-Time Packing (Density Win)**: For request-bounded tools whose input size `n` is known at arrival (parsers, BPE tokenizers, JSON-schema masks, protocol decoders, bounded tool calls), the scheduler evaluates the proved parametric formula `(heap(n), stack, ops(n))` on the actual incoming request size `n` and packs tenants tightly with zero static container guesswork.
- **Mode 2 -- Safe Overcommit via `FillFailed` (The Edge Over Docker and Wasm)**: For bursty run-produced work (LLM output streams, multi-step agent loops), the scheduler packs tenants by typical average usage. If a rare burst hits the host's memory cap, Stigi does **not** OOM-kill the worker or trap mid-tool-call. Allocation returns typed `FillFailed`, enters a failure handler that was already exercised by fault-injection tests, walks every open file/socket/transaction cleanly to its stop state, and yields back to the scheduler.

<details>
<summary><strong>Example: What a scheduler receives from Stigi's admission verdict</strong></summary>

```text
unit:       Tools.JsonSchema.validatePayload
runtime_id: b3:8f41c2...9a0e
authority:  none (pure; Region only)
stack:      640 bytes (proved, signal reserve included)
time:       14 * inputBytes + 32 ops   [O(n)]
heap:       2 * inputBytes + 128 bytes [flat; 0 retained dead bytes]
failures:   FillFailed (tested), InvalidUtf8 (tested), SchemaMismatch (tested)
protocols:  none open on any exit path
```

</details>

### Zero-Hypervisor Deterministic Replay and Multiverse Forking

Workflow engines like Temporal or Restate ask developers to follow determinism rules on the honor system: accidentally call `Date.now()`, read an environment variable, or use an unordered hash map, and replay blows up in production. Hypervisor-based simulation tools solve determinism by recording an entire Linux VM -- at massive compute and startup cost.

Because Stigi forbids ambient authority and models the LLM, clock, filesystem, network, and concurrency scheduler as explicit capabilities, **recording the boundary journal captures 100% of external nondeterminism automatically**:

- **Byte-identical replay without a hypervisor**: Swap the live handlers for the journal-replay handler and re-execute thousands of agent runs in-process at native C speed with zero nondeterminism errors.
- **Step-`k` multiverse forking**: Fork an agent session at step `k` (injecting a simulated tool timeout, a different memory limit, or an alternate LLM completion) without snapshotting process memory or booting a microVM. Recorded effects before step `k` never re-execute; live calls after step `k` branch cleanly from explicit versioned state.

### How Stigi Compares to Existing Runtimes

| Runtime / System | Authority in Types | Parametric Resource Envelope + Safe Overcommit | No Mid-Effect Death / Linear Stop States | Complete Effect Journal (Replay & Fork) |
|---|---|---|---|---|
| **Docker / Firecracker** | OS sandbox (runtime) | Static caps (OOM kill on breach) | No (SIGKILL mid-effect) | No (VM snapshots only) |
| **WASI / Wasm Components** | Runtime import list | Fuel & memory traps | No (traps mid-effect) | No |
| **Temporal / Restate + Wasm** | Runtime host bindings | Runtime timeouts/limits | No (traps/OOM mid-effect) | Honor-system replay |
| **Linux eBPF** | Verified helper whitelist | Static stack + verifier cap | No algebraic protocols | No |
| **Erlang / BEAM** | No (ambient I/O) | Runtime reductions + per-process heap | "Let it crash" drops external protocols | No |
| **Unison** | Yes (abilities + hash shipping) | No (garbage collected) | No linear stop-state proof | Partial |
| **Stigi** | **Proved at admission** | **Proved stack/cost/heap + `FillFailed` overcommit** | **Proved stop states + fault-tested arms** | **Complete by construction** |

---

## 7. Tokenomics First: Built for AI-Authored Software

In AI-native software engineering, **tokens are literally your COGS.** Yet every mainstream programming language and toolchain was designed for humans typing into files and reading scrolling terminal logs.

### Why We Compress Reads and the Protocol -- Not Write Syntax

A lot of experimental "AI languages" try to cut token bills by inventing terse glyph syntax, single-letter aliases, or raw JSON ASTs. Empirical research (like SimPy and Token Sugar) shows why that backfires for zero-shot frontier models: weird syntax saves at most ~10--13% per file while breaking the model's pretraining priors, inflating hidden thinking tokens, and doubling compiler-retry loops -- **raising the total dollar bill.**

Look at where an agent's token bill actually goes across a multi-turn task:

1. **Context discovery** (running `grep` and slurping whole files just to find three function signatures),
2. **Whole-file rewrites** (streaming 400 lines of output tokens to fix a 3-line bug),
3. **Multi-turn compile-retry loops** (counting string byte lengths by hand, guessing missing preconditions, or hitting cascading errors),
4. **Verbose, repeated diagnostics** (re-sending walls of compiler text and JSON keys every turn), and
5. **Broken prompt caches** (timestamps, changing orderings, or drifting tool schemas that destroy 90% cache-read discounts).

Stigi exploits a clean asymmetry: **because the checked image is independent of text syntax, we compress everything the door SENDS (reads, queries, diagnostics) while keeping natural, high-prior Scala 3 ASCII for everything the agent WRITES.**

- **Readable names and prompts stay law**: Full English words tokenize as single BPE tokens anyway (unlike fragmented abbreviations) and keep code readable for humans; every unit keeps its `// prompt:` line.
- **Zero-prior-loss write ergonomics**: String literal lengths are inferred automatically (`Array[U8](_) = "hello"` -- killing off-by-one byte-counting retries), and `batch` frames support wire-only `alias` headers expanded cleanly before admission so agents don't repeat long dotted namespaces while stored units stay crystal clear.
- **Path-addressed structural edits**: Instead of rewriting a 120-line unit, agents patch exact paths -- `edit Parser.step.arm[TokColon] := ...`, `edit Parser.step.facts := ...`, or `edit Parser.step.body[12..15] := ...` -- so output tokens scale with the *change*, not the unit.
- **Measured in CI like CPU speed**: Every compiler change is gated on **tokens-to-green** (split into cached input, fresh input, and output/thinking tokens), **repeat-rejection rate**, and **session cache-hit rate**.

### How an Agent Session Actually Works in Stigi

Agents talk directly to the live Stigi image over local Unix sockets or native MCP -- zero shell calls, zero filesystem reads, zero `grep`:

```text
# 1. Session starts with a byte-stable cached prefix:
#    agent reference + static lesson table + zoomed abstract card

$ stigi zoom Billing L0
# ~110 tokens: exact capability manifest, exercised protocols, laws, invariants, envelopes

# 2. Agent requests the exact 1-turn editing slice for one function:
$ stigi slice Billing.Invoice.applyDiscount --depth 1
# Returns ONLY Billing.Invoice.applyDiscount + exact signatures/<facts> of its
# 4 direct callees, 2 callers, 2 enums, and 1 touching law -- sorted deterministically.

# 3. Agent sends a compact, path-addressed edit with a wire-only batch alias:
$ stigi batch
alias Inv = Billing.Invoice
edit Inv.applyDiscount.arm[Percentage(rate)] :=
  case Percentage(rate) if rate <= 100 => Inv.scaleDown(total, rate)

# 4. If a check fails, cascading errors collapse to ONE root missing fact,
#    with a counterexample, a pre-checked fix, and a cached-prefix lesson pointer:
rejected {Billing.Invoice.applyDiscount}: precondition-unproved (1 root, 3 downstream collapsed)
  need: below(rate, 101) at Billing.Invoice.applyDiscount.arm[Percentage]
  counterexample: total=500, rate=101 -> violates Inv.scaleDown entry fact
  fix1 (checked): refine Percentage field with <lessOrEqual(rate, 100)>
  lesson: precondition-unproved (see cached prefix)

# 5. Agent accepts the checked fix in two tokens:
$ stigi accept fix1
admitted Billing.Invoice.applyDiscount   laws ok   tests 42/42   shaken: 0
```

Columnar compact formatting cuts read payload tokens sharply on `summary`, `query`, and diagnostic tables while keeping every reply byte-identical for maximum prompt-cache hit rates. And when the door finds purely mechanical cleanup (like shaking unused binders), it offers a one-shot semantics-preserving `minimal:` fix -- while never silently inserting runtime guards behind the agent's back.

---

## 8. Understanding What Code Does Without Reading Slop: The Proved Theory Sheet

This is the problem that decides whether humans remain in control of AI-built software.

### Why Giant Visual Graphs Fail -- and What Humans Are Actually Good At

When codebases grow past human reading speed, people instinctively try to render them as node-and-wire diagrams. That is a dead end. Decades of human-factors research (and every engineer who has ever opened an auto-generated UML diagram) show the same thing: **human working memory breaks past roughly fifty visual primitives on screen.** Turning 5,000 lines of AI code into 5,000 boxes and 12,000 crossing arrows simplifies nothing.

Humans are bad at holding ten thousand concrete execution details in memory -- we forget. **What humans are extraordinarily good at is abstract mathematical thinking**: boundaries, state machines, invariants, exhaustive tables, and theorems.

Simplification has to happen **mathematically** before anything is printed on screen. Because Stigi's door already proves the structure, effects, invariants, and laws of the entire image, Stigi projects any subsystem into a **Proved Theory Sheet** (`abstract`) whose size grows with the **ideas** in the system, never with the lines of implementation underneath.

### Inside a Proved Theory Sheet

Ask Stigi to explain a 4,000-line payment worker (`stigi abstract Payments.Worker.main`), and instead of dumping 80 files of concrete loops, Stigi renders six compact mathematical views straight from door-derived facts:

```text
$ stigi abstract Payments.Worker.main
========================================================================
THEORY SHEET: Payments.Worker.main   (312 units collapsed, 0 ambient I/O)
========================================================================

[1. AUTHORITY, INVARIANTS & ENVELOPE — PROVED]
  Capabilities:  Files[write-temp-rename under ./ledger], Socket[TLS ledger.internal:443], Clock[mono]
  Trust surface: 2 counted C externs (posix_fsync, tls_write), 0 user axioms
  Invariants:    Cents(amount) >= 0; BatchSize <= 256; AccountIndex < accountCount
  Envelope:      stack <= 1,840 B | time <= 22*entries + 90 ops [O(n)] | heap <= 64*entries + 4 KiB [flat]

[2. CHECKED SUMMARY — CLAIM-ANCHORED]
  [PROVED]  Never opens network connections outside ledger.internal:443.   (@claim manifest)
  [PROVED]  Every ledger write ends in Committed or RolledBack on all paths (@claim protocol)
            including FillFailed and socket disconnects.
  [TESTED]  Matches pure reference model Spec.Ledger.applyBatch across 10,000 seeded boundary cases.
  [UNCHECKED PROSE] Implements idempotent end-of-day settlement for EU merchant accounts.

[3. EXERCISED CAPABILITY PROTOCOLS — PROVED]
  LedgerFile:  Init --[begin]--> Staging --[append*]--> Staging
               Staging --[commit]--> Committed (stop)
               Staging --[FillFailed | IoErr -> rollback]--> RolledBack (stop)
  Denials:     NEVER close(Socket) while LedgerFile is Staging

[4. LAWS & EXHAUSTIVE PUBLIC DECISION TABLE — PROVED / DISCOVERED]
  Laws (kernel-only, 0 axioms, proof bodies folded):
    - Ledger.balanceConserved : sum(after) == sum(before)   [PROVED]
    - Ledger.feeBounded       : fee <= amount / 20          [PROVED]
  Public Table: Payments.classifyTransfer(amount, dailyUsed, limit)
    | condition                                  | outcome         | exit fact / effect        |
    |--------------------------------------------|-----------------|---------------------------|
    | amount == 0                                | RejectZero      | pure                      |
    | amount > 0 && dailyUsed + amount <= limit  | Approve(amount) | total <= limit, pure      |
    | amount > 0 && dailyUsed + amount > limit   | HoldForReview   | pure                      |
  Discovered Algebraic Laws (`discover`):
    - Codec.decode(Codec.encode(entry)) == Ok(entry)        [TESTED 10k — ready to promote]

[5. INFORMATION-FLOW MATRIX — PROVED (explicit + control flow, whole-array over-approx)]
  Source \ Sink      | LedgerFile.write | Socket.write | AuditLog.write
  -------------------+------------------+--------------+---------------
  Socket.read        | YES              | YES          | YES
  Config.secretKey   | NO (disjoint)    | YES (TLS)    | NO (disjoint)
  Clock.monotonic    | NO (disjoint)    | NO (disjoint)| YES
```

Look closely at what that single screen gives a human reviewer or architect:

- **Zero lying docstrings or hallucinated AI summaries.** Notice section `[2]`: when an AI writes a human explanation of a module, every factual claim embeds a live `@claim(...)` anchor checked by the door. If the AI hallucinates *"never touches the network"* or a later refactor quietly adds an AuditLog write from `secretKey`, the anchor immediately fails (`claim-false` / `explanation-stale`). Any sentence without a formal anchor is explicitly stamped `[UNCHECKED PROSE]` so prose can never impersonate proof.
- **Exhaustive Parnas decision tables without reading nested `match`/`if` trees.** Public functions automatically project their exhaustive decision trees into compact, disjoint condition-to-outcome tables (and if a table exceeds a clear readability cap, Stigi reports `table-too-large` instead of silently hiding rows).
- **Automatic algebraic law discovery (`discover`).** When an agent builds a new codec, cache, or parser, Stigi explores small pure expressions over its public API to surface clean mathematical laws (`decode(encode(x)) == Ok(x)`, idempotence, commutativity, monotonicity) as `TESTED` conjectures that humans can inspect and promote into permanent laws.
- **Zooming out never lies.** Like Galois abstraction in static analysis, zooming out forgets internal helper details without ever inventing false guarantees.

### Reviewing Theory Diffs Instead of 800-Line Text Diffs

When an AI agent refactors a subsystem across 25 units, Stigi's `diff` compares the **Theory Sheets** before and after:

```text
$ stigi diff --from prev --to head Payments.Worker
semantic delta across 25 edited units:
  authority:     UNCHANGED
  protocols:     UNCHANGED
  info-flow:     UNCHANGED
  laws:          2/2 PROVED, 0 weakened
  trust surface: UNCHANGED (0 new externs, 0 new wrapping ops)
  envelope:      time 22*n+90 -> 14*n+64 (-36%), stack 1840 -> 1408 B (-23%)
  shaken slop:   4 dead helpers removed, 11 constant folds
  refactors:     19 units pure extract/inline invariant (0 semantic change)
```

A refactor that changes zero authority, zero protocols, zero information flow, zero laws, and shrinks time and stack by 30% takes **ten seconds** for a human to review with confidence. Add one new outbound host, one wrapping arithmetic bypass, or one flow edge from `secretKey` to `AuditLog`, and it lights up on line one.

### Why Testing in Stigi Is Not "Running Untrusted Code"

People understandably ask: *"Wait -- you still have tests, aren't you still testing by running?"*

In Stigi, two crucial things change:

1. **Tests have zero authority and run purely at compile time.** Every test runs inside Stigi's pure, bounded compile-time evaluator with no filesystem, no network, no environment variables, and no external syscalls (external effects are swapped for local deterministic handlers). Running untrusted AI code in a Stigi test literally cannot touch your machine.
2. **Tests are never asked to prove safety.** Memory safety, bounds, arithmetic, stack limits, permissions, resource protocols, complexity, and algebraic laws are **proved without running anything**. Pure compile-time property tests are reserved strictly for content-level functional equivalence against tiny human-owned reference models -- and every result stays explicitly labeled `TESTED`, never conflated with `PROVED`.

---

## 9. Parallelism, Native C Interop, Embedded Boards, WebAssembly, and GPUs

Stigi keeps **one checked intermediate representation** across radically different hardware profiles:

- **Parallel by default, deterministic by law**: Because single-writer aliasing already holds in single-threaded code, Stigi's parallel thread pools, structured concurrency scopes (`when` multi-resource exclusion, zero raw locks), non-blocking completion I/O (`io_uring` linear tickets), and actor/remote handlers produce results **byte-identical to sequential execution**. The compiler can even auto-parallelize pure calls over disjoint owned data while verifying bit-for-bit equality against the sequential oracle.
- **Modern functional ergonomics compiled down to flat C**: Multi-head deep pattern matching, functional pipes (`|>`), zero-overhead monomorphized traits and implicits, defunctionalized lambdas with exact call graphs, multi-shot effect handlers where needed, generation-checked slot tables (eliminating stale-index bugs), 128-bit integers, deterministic balanced-tree floating-point reductions, and fixed-layout byte-order records (`Bytes.view` with proved ranges).
- **Drop-in verified C libraries (`export "c"`)**: You don't have to rewrite your entire company in Stigi to get value on day one. Export any Stigi module as a clean C static/shared library with a generated header -- zero panics, zero OOM crashes, bounded stack, zero ambient I/O -- and call it directly from Python, Rust, Go, or C++. Practical drop-ins like **verified LLM BPE tokenizers** and **JSON-schema constrained-decoding mask engines** slot straight into existing AI serving stacks.
- **Content-addressed code identity**: Every unit is hashed by its post-erasure computation structure and runtime capabilities, not by text or variable names. Renaming a function or variable is free; moving code across peers over content-addressed stores never trusts a remote peer's claim -- the receiver re-checks through its own door.
- **From microcontrollers to GPUs**: Target profiles swap integer widths, byte order, stack budgets, and hardware extern tables without changing a single safety rule:
  - **Linux servers** (x86-64, aarch64, riscv64; standard eager-populated pools or strict memory-locked realtime profiles),
  - **Freestanding embedded boards** (no libc, device capabilities only, tight proved stack budgets),
  - **WebAssembly components** (capability row equals Wasm import list for double-enforced sandboxing), and
  - **Data-parallel integer GPU kernels** (proved affine tile partitioning and barrier typestate eliminating shared-memory data races at compile time).

---

## 10. Self-Hosting, Independent Certification, and Exact Limits

Trust in a verification language comes from checking the checker itself -- never from asking people to take our word for it:

- **Self-hosted to a byte-identical fixpoint**: Stigi compiles itself (`stigi.1 -> C2 -> stigi.2 -> C3`, with `C2 == C3` byte for byte across GCC, Clang, and memory-safe Fil-C). That fixpoint proves the compiler binary faithfully matches its reviewed source tree.
- **External, non-circular soundness proofs**: A fixpoint alone proves faithful compilation, not mathematical soundness. Stigi's Datalog admission certificates and language soundness theorems are verified externally in Lean and cross-checked by independent proof checkers -- never circularly by a Stigi-compiled kernel.
- **Continuous multi-compiler backstop**: Emitted C is compiled with `-fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks` so even a hypothetical checker bug cannot turn into optimizer-exploited undefined behavior, and tested under memory-safe Fil-C where every object gets its own checked allocation.

### What Stigi Does Not Claim

Every strong guarantee has sharp edges. Here are Stigi's exact boundaries:

- **Sound and conservative**: Stigi rejects some safe programs whose proofs fall outside its decidable Datalog logic. That is a deliberate trade -- fast, predictable, deterministic checking instead of hanging SMT solvers.
- **Physical and platform exclusions**: No user-space language can prevent an external `SIGKILL`, sudden power loss, hardware bit flips, or defects in the operating system kernel, C library, C compiler, or listed `extern "c"` C bindings (every external C binding is explicitly counted and audited in `trust`).
- **Protocol completion vs external crash atomicity**: Proving a file handle reaches `Closed` guarantees the resource protocol finished cleanly -- it does not automatically make raw OS writes crash-atomic unless the capability protocol itself models commit and rollback (like write-temp-then-rename).
- **Human business intent**: Static proofs tell you what a program *can touch*, *how fast and how much memory it uses*, *what invariants hold*, and *what cannot leak*. Deciding whether a product specification is the right thing to build for your users always stays with humans -- through small reference specs, laws, checked explanations, and unit prompts.

---

## 11. Sponsor, Early Adopters, and License (AGPL)

Stigi is not backed by a big AI lab, a venture fund, or a single corporation. It is built by one person -- [Victor Borja (`@vic`)](https://oeiuwq.com), a developer from Mexico City who has been writing open-source software since 1999, loves compilers, Scala, effect systems, and Nix, and builds tools for doers and engineers to solve real modern problems. Almost everything I know I learned by reading other people's code, and giving useful work away in the open has always been an act of love for me.

### Why Stigi Is AGPL

As AI generates more and more of the software the world runs on, the tools that verify, bound, and explain that software shouldn't belong to one closed vendor. Stigi is licensed under the **GNU AGPL** so it stays free, auditable, and guaranteed for everyone -- never enclosed or controlled by a single company. Anyone can read the rules, audit the proofs, build on top of it, and know the core compiler stays improving in the for the good for all. Same as with the Linux kernel, companies can build upon it.

### Sponsoring Stigi

Building a self-hosted verified compiler, running multi-compiler and memory-safe test backstops, and iterating with frontier models takes real compute, hardware, and sustained full-time focus. Today I pay for all of those resources out of my own pocket.

If Stigi matters to you or your team, money is simply the practical way to sustain the work and the infrastructure it takes to keep shipping fast, reliable releases:

- **Sponsor directly on GitHub**: [github.com/sponsors/vic](https://github.com/sponsors/vic)
- **About Victor and other open-source projects**: [oeiuwq.com](https://oeiuwq.com)

Every sponsor -- individual or company -- is credited by name in each Stigi release.

### Looking for First Early Adopters

Just as important as funding -- and even more valuable right now -- are **first early adopters**.

If you are an engineer, researcher, or small team willing to put Stigi to work on a real problem (a verified C library dropped into your Python/Rust/Go stack, a tight parser or schema engine, an agent tool runtime, or a systems utility), try it out, push on the rough edges, file issues when a rule rejects something sensible, and tell us what breaks. Real users trying to ship real work are what turn a good compiler idea into something people rely on every day.
