---
title: Testing is not running untrusted code
description: Why Stigi tests are safe to run, and what they are for.
sidebar:
  order: 4
---

A fair question: *"You still have tests. Aren't you still testing by running?"*

Yes, but two things are different.

## Tests have no power

Every Test runs inside Stigi's pure, bounded compile-time evaluator. No file system, no network, no environment variables, no system calls. External effects are replaced by local, deterministic handlers. Running AI-written code in a Stigi test cannot touch your machine.

## Tests are never asked to prove safety

Memory safety, bounds, arithmetic, stack, permissions, protocols, complexity and laws are all **proved**, without running anything. Tests are kept for what proofs over facts do not cover: does this function give the right content, compared to a small reference model a human wrote?

## Two labels, never mixed

Every guarantee in Stigi carries a label:

- **PROVED**: decided by the door's rules, for every input.
- **TESTED**: checked on many seeded cases, near the edges where bugs live.

A theory sheet, a diff or a doc always says which one it is. A tested property is never presented as a proved one.

That is the human side. Next, what all this gives you in production: [the runtime envelope](/runtime/envelope/).
