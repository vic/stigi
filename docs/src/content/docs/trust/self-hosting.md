---
title: Checking the checker
description: Self-hosting, external proofs and a multi-compiler backstop.
sidebar:
  order: 1
---

Trust in a verification language has to come from checking the checker. Not from taking our word for it.

## Stigi compiles itself

The Stigi compiler is written in Stigi. It compiles itself to a byte-identical fixpoint:

```text
stigi.1 -> C2 -> stigi.2 -> C3      C2 == C3, byte for byte
```

That holds across GCC, Clang and the memory-safe Fil-C. It shows the compiler binary matches its reviewed source tree.

## Soundness is proved outside

A fixpoint shows faithful compilation, not that the rules are sound. So the admission certificates and the language soundness theorems are checked in Lean, and cross-checked by independent proof checkers. Never by a kernel that Stigi compiled itself: that would be circular.

## A backstop under everything

- Emitted C uses flags that remove the undefined behavior optimizers love to exploit.
- Every build also runs under Fil-C, where each object gets its own checked allocation.
- Adversarial suites attack the door with sanitizers, exact arithmetic checks, hardware stack guards and a hostile NegTest for every kernel unit and every C binding. Every break they find becomes a permanent test.

Next: [what Stigi does not claim](/trust/limits/).
