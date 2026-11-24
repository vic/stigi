---
title: No panics, no overflow, no stack explosions
description: The crash paths other languages hide, and how Stigi closes them.
sidebar:
  order: 4
---

Almost every standard library hides crash paths: `unwrap`, bounds traps, slice panics, stack overflows, allocation aborts. Stigi has none of them, standard library included.

## Arithmetic

Every `+`, `-`, `*`, `/`, `%` and shift must be proved to stay inside its type. When you really want wrapping, for a hash for example, you call a named operation like `U64.wrappingAdd`. Every new wrapping operation shows up in review diffs.

## Termination and recursion

Every call cycle proves that it ends, or that its depth is bounded. Tail recursion becomes a flat loop with constant stack.

## Stack

Every root function has a proved worst-case stack size. It adds up every Stigi frame on the deepest path, the declared stack cost of each C call on that path, and the space the OS needs for signals. The emitted C is cross-checked against the compiler's own stack-usage tables.

## Out of memory

Allocation returns `FillFailed` as a value, and the program handles it like any other case. See [errors are values](/language/errors/).

## The result

The emitted C has zero `abort(` and zero `exit(` calls. A Stigi program does not die on its own. It returns.

Next: [no ambient authority](/door/authority/).
