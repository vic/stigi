---
title: What Stigi does not claim
description: The exact edges of every guarantee.
sidebar:
  order: 2
---

Every strong guarantee has sharp edges. Here are ours.

## It rejects some safe programs

Stigi's logic is decidable on purpose. Some safe programs need proofs outside it, and Stigi rejects them. We take that trade: fast, predictable checks that always give the same answer, instead of solvers that hang.

## It cannot stop the physical world

No language can stop an outside `SIGKILL`, a power cut, a flipped bit in RAM, or a bug in the OS kernel, the C library, the C compiler or a listed C binding. Every C binding is counted and shown in `stigi trust`.

## Closed is not the same as atomic

Proving a file reaches `Closed` means the protocol finished cleanly. It does not make raw writes crash-safe on its own. For that, the protocol has to model commit and rollback, like write-to-temp-then-rename.

## Intent stays with people

Proofs tell you what a program can touch, how fast it runs, how much memory it uses, what invariants hold and what cannot leak. Whether it is the right thing to build for your users is a human decision. Stigi helps with small reference specs, laws, checked docs and unit prompts, but the decision stays yours.

Want to help? See [sponsor, adopt, license](/community/sponsor/).
