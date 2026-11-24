---
title: Checked complexity
description: O(n) as a proved fact, with the hidden constants in plain sight.
sidebar:
  order: 6
---

A type signature tells you what goes in and what comes out. It says nothing about whether a function takes ten steps or ten billion. In Stigi, cost is a fact like any other.

```scala
// prompt: find the index of a target byte in a sorted block in logarithmic time.
pub def Search.binaryFind(size: U64, block: Array[U8](size), target: U8)
    <lessOrEqual(1, size), cost(O(log size))>: Search.Lookup =
  Search.binaryStep(size, block, target, 0, size)
```

## Exact first, then O(...)

Plain big-O is easy to fool: a loop of 1,000,000,000 fixed steps is technically `O(1)`. So Stigi first proves exact 128-bit step and allocation coefficients, and only then shows the leading term as `O(1)`, `O(log n)`, `O(n)`, `O(n log n)` or `O(n^2)`. The big constant is there for everyone to see.

- Halving loops and balanced divide and conquer give `O(log n)` and `O(n log n)`.
- An uneven split that turns quadratic is reported as `O(n^2)`.
- Allocations, array copies and C calls all carry a cost. Work cannot hide inside them.

Cost lives in the same Datalog engine as bounds and termination. There is no SMT solver and no LP solver in the checker, so it stays fast and gives the same answer every time.

## Amortized cost

A growing buffer pays `O(n)` now and then, but `O(1)` on average. A state record declares one potential function, and the door checks that every step stays within the amortized bound:

```scala
// prompt: append one byte to a doubling buffer in amortized constant time.
pub def Buffer.pushByte {region: Stigi.Region, Stigi.Mut}
    (state: Buffer.Growing, byteValue: U8) <amortized(O(1))>: Buffer.PushOutcome =
  ...
```

Doubling passes. Growing by one at a time is rejected.

Next: [search by contract](/door/search/).
