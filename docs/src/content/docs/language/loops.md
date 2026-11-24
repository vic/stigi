---
title: Loops and invariants
description: while loops, loop invariants and termination.
sidebar:
  order: 4
---

Stigi has one loop: `while`. There is no `for`, no `break` and no `continue`. Small, plain loops are easy to read and easy to prove.

```scala
// prompt: the sum of the first count numbers in a table, capped by a limit.
def Demo.sumUpTo(count: U64, table: Array[U64](count), limit: U64): U64 <lessOrEqual(result, limit)> =
  var total: U64 = 0
  var index: U64 = 0
  while index < count do
    @<lessOrEqual(total, limit), lessOrEqual(index, count)>
    val value = table(index)
    total = if value <= limit - total then total + value else limit
    index = index + 1
  total
```

## The loop invariant

A bare `@<...>` on the first line of a loop body is the **loop invariant**. The door proves it when the loop starts and again at the end of every turn. In return, you can rely on it inside the body and after the loop.

Here it gives the exit fact `lessOrEqual(result, limit)` without a single runtime check. The guard `index < count` makes `table(index)` safe, and the `if` makes the addition safe.

## Termination

Every loop and every recursive call must end. Stigi proves it from a value that keeps getting smaller, like `count - index` above. Recursion is allowed when its depth is bounded, and tail recursion becomes a flat loop with constant stack.

Next: [enums and pattern matching](/language/enums/).
