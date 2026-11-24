---
title: "Projects: verification tools"
description: Proof tools, checkers and analyzers that relate to Stigi.
sidebar:
  order: 3
---

Stigi checks its programs with proofs, so it stands on a lot of verification work. These projects show the tools and ideas it learns from.

## Proof assistants and checkers

- [Agda](https://wiki.portal.chalmers.se/agda/) — dependently typed language and proof assistant. Stigi shares its dependent types.
- [Alpöge RH/GRH archive](https://alpo.ge/) — machine-checked Lean proofs of RH and GRH. Millions of checked lines that no human has read: the reason Stigi builds theory sheets.
- [Aristotle](https://duckduckgo.com/?q=Aristotle+Lean+prover) — LLM-based Lean prover. Stigi checks what such provers produce.
- [CoLoR](https://duckduckgo.com/?q=CoLoR+Coq+termination+library) — Coq library for termination. Stigi shares the idea of a certified checker.
- [con-leche](https://duckduckgo.com/?q=con-leche) — external Lean proof checker. Stigi cross-checks against it.
- [CVC4](https://cvc4.github.io/) — SMT solver. Stigi keeps SMT solvers out of its trusted checker.
- [Dafny](https://dafny.org/) — verification-aware language. Stigi also checks specifications with programs.
- [F*](https://fstar-lang.org/) — proof-oriented language. Stigi shares its mix of programs and proofs.
- [formal-math](https://github.com/anthropics/formal-math) — Lean proof archive. A reminder that checked is not the same as understood.
- [GuessLex](https://lean-lang.org/) — termination tactic in Lean 4. Stigi learns from how it picks measures.
- [Hoverfly](https://duckduckgo.com/?q=Hoverfly+Lean+proof+tree) — proof-tree UI for Lean. Stigi shares the goal of readable proofs.
- [Iris](https://iris-project.org/) — separation logic framework. Stigi takes ideas from its resource reasoning.
- [Isabelle](https://duckduckgo.com/?q=Isabelle+theorem+prover) — interactive theorem prover. Stigi learns from its long record of checked proofs.
- [IsaFoR / CeTA](https://cl-informatik.uibk.ac.at/software/ceta/) — certified termination checking. Stigi shares the idea of a certified checker.
- [Lean 4](https://lean-lang.org/) — theorem prover and programming language. Stigi checks its soundness proofs in Lean 4.
- [Lean Kernel Arena](https://github.com/leanprover/lean-kernel-arena) — test corpus for Lean kernels. Stigi uses its tests to compare Lean checkers.
- [lean-smt](https://github.com/ufmg-smite/lean-smt) — SMT proof replay in Lean. Stigi shares the idea of replaying solver proofs in a small kernel.
- [Lean4Lean](https://github.com/digama0/lean4lean) — verified Lean kernel checker. Stigi shares its NbE-based approach to checking.
- [LeanArchitect](https://github.com/hanwenzhu/LeanArchitect) — Lean blueprint tool. Stigi shares the idea of plans tied to proofs.
- [LRAT](https://github.com/marijnheule/drat-trim) — SAT proof format. Stigi checks certificates in this style.
- [Magmide](https://github.com/Magmide/magmide) — proofs for bare-metal code. Stigi shares the goal of proofs down to the metal.
- [Mathlib](https://github.com/leanprover-community/mathlib4) — Lean mathematics library. The largest shared body of checked mathematics.
- [nanoda](https://github.com/ammkrn/nanoda_lib) — external Lean kernel checker. Stigi cross-checks against it.
- [PBLean](https://duckduckgo.com/?q=PBLean) — pseudo-Boolean proof replay in Lean. Stigi shares the idea of replaying solver proofs in a small kernel.
- [Rocq](https://rocq-prover.org/) — proof assistant, formerly Coq. Stigi learns from its extraction and proof practice.
- [smalltt](https://github.com/AndrasKovacs/smalltt) — fast NbE elaborator. Stigi learns from its normalization design.
- [sokonanoda](https://duckduckgo.com/?q=sokonanoda) — Lean checker in Rust using NbE. Stigi cross-checks against it.
- [tenet](https://github.com/keithadler/tenet) — Lean .olean checker in C#. Stigi cross-checks against it.
- [VeriPB](https://gitlab.com/MIAOresearch/software/VeriPB) — proof checker for pseudo-Boolean solvers. Stigi checks certificates in this style.
- [Verso blueprint](https://github.com/leanprover/verso-blueprint) — blueprint tooling for Lean. Stigi shares the idea of plans tied to proofs.
- [Z3](https://github.com/Z3Prover/z3) — SMT solver. Stigi keeps SMT solvers out of its trusted checker.

## Verifiers and analyzers

- [Alloy](https://alloytools.org/) — lightweight model checker. Stigi shares the small-model approach to finding errors.
- [AWS Cedar](https://www.cedarpolicy.com/) — authorization language built with verification. Stigi shares its verification-guided development.
- [Cerberus](https://www.cl.cam.ac.uk/~pes20/cerberus/) — ISO C semantics model. A precise model of what C programs mean.
- [Clippy](https://github.com/rust-lang/rust-clippy) — Rust linter. Stigi shares the idea of catching mistakes early.
- [CN](https://github.com/rems-project/cerberus) — C refinement verifier. Stigi shares its refinement approach for C.
- [CompCert](https://compcert.org/) — verified C compiler. Stigi learns from its proof of compiler correctness.
- [Creusot](https://github.com/creusot-rs/creusot) — Rust verification tool. Stigi shares the goal of proving Rust-like code.
- [Flux](https://github.com/flux-rs/flux) — liquid types for Rust. Stigi also uses liquid-style refinements.
- [GPUVerify](https://github.com/mc-imperial/gpuverify) — GPU kernel race verifier. Stigi shares the aim of race freedom checked by tools.
- [GraphicsFuzz](https://github.com/google/graphicsfuzz) — shader compiler fuzzer. Stigi borrows its idea of testing compilers by variants.
- [Kani](https://github.com/model-checking/kani) — Rust model checker. Stigi shares the goal of bounded proofs for real code.
- [Liquid Haskell](https://ucsd-progsys.github.io/liquidhaskell/) — refinement types for Haskell. Stigi uses refinement types in the same way.
- [Polonius](https://github.com/rust-lang/polonius) — Datalog-based Rust borrow checker. Stigi also checks ownership with Datalog.
- [Propel](https://duckduckgo.com/?q=Propel+theory+exploration) — theory exploration tool. Stigi shares the idea of finding laws automatically.
- [QuickChick](https://github.com/QuickChick/QuickChick) — property-based testing for Rocq. Stigi shares property tests next to proofs.
- [QuickSpec](https://github.com/nick8325/quickspec) — algebraic law discovery. Stigi shares the idea of finding laws from examples.
- [RAML](https://www.raml.co/) — resource-bound analysis for OCaml. Stigi learns from its cost analysis.
- [RefinedC](https://plv.mpi-sws.org/refinedc/) — C refinement verifier. Stigi shares its refinement approach for C.
- [Spin](https://spinroot.com/) — model checker. Stigi shares its protocol checking.
- [TiML](https://duckduckgo.com/?q=TiML+time+complexity+types) — time-complexity type system. Stigi shares the idea of cost in types.
- [VeriLLVM](https://duckduckgo.com/?q=VeriLLVM) — LLVM verification tool. Stigi shares the aim of checking compiler output.
- [Verus](https://github.com/verus-lang/verus) — verified Rust. Stigi shares the goal of proofs beside code.

## Datalog and synthesis

- [Ascent](https://github.com/s-arash/ascent) — Datalog in Rust. Stigi also embeds Datalog in its checks.
- [Barliman](https://github.com/webyrd/Barliman) — relational program synthesizer. Stigi shares its relational search.
- [DBSP](https://github.com/feldera/feldera) — incremental view maintenance. Stigi shares the idea of incremental results.
- [egglog](https://github.com/egraphs-good/egglog) — Datalog with equality saturation. Stigi shares the mix of Datalog and equality.
- [lemmalog](https://duckduckgo.com/?q=lemmalog) — Datalog for agent memory. Stigi keeps all its facts in Datalog too.
- [Logica](https://logica.dev/) — Datalog-based language. Stigi shares the idea of logic rules as programs.
- [Myth](https://duckduckgo.com/?q=Myth+type+directed+program+synthesis) — type-directed synthesizer. Stigi shares the idea of types guiding search.
- [Resyn](https://duckduckgo.com/?q=Resyn+resource+guided+synthesis) — cost-guided synthesizer. Stigi shares the idea of cost-guided search.
- [Souffle](https://souffle-lang.github.io/) — Datalog engine and compiler. Stigi runs Datalog in a similar way.
- [Synquid](https://duckduckgo.com/?q=Synquid+program+synthesis) — refinement-type synthesizer. Stigi shares the idea of types guiding search.
- [μKanren](https://github.com/jasonhemann/microKanren) — small relational core. Stigi builds its relational search on the same core.

## C safety and sanitizers

- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) — memory error detector. Stigi finds the same errors by types, before running.
- [Fil-C](https://fil-c.org/) — memory-safe C compiler. Stigi shares the goal of memory-safe C.
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) — undefined behavior detector. Stigi rules out the same cases by types.
