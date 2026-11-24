---
title: Parallel by default
description: Parallel code that gives the exact same answer as sequential code.
sidebar:
  order: 2
---

Stigi already enforces a single writer for every piece of data, even in single-threaded code. That is the hard part of parallelism, done up front.

So parallel code in Stigi is deterministic:

- **Thread pools and structured scopes.** A `when` scope takes several resources at once, with no raw locks.
- **Completion-based I/O.** `io_uring` operations are linear tickets that must be completed.
- **Actors and remote handlers.** Same capabilities, same checks.

Every one of these gives results **byte-identical** to running the same code in sequence. The compiler can even run pure calls over disjoint owned data in parallel by itself, and check the result bit for bit against the sequential run.

## Determinism everywhere

Determinism is not only about threads. Every verdict of the door, every output and every reply to an agent is deterministic too. Same image, same question, same bytes.

Next: [from microcontrollers to GPUs](/platforms/targets/).
