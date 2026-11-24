---
title: Memory without a garbage collector
description: Single ownership, freed at last use, with nothing to annotate.
sidebar:
  order: 7
---

Fast systems code usually makes you pick a pain. Garbage collector pauses (Go, Java). Manual `free` bugs (C, Zig). Or lifetime annotations and `Rc` wrappers (Rust). Stigi takes another path.

## One owner, inferred

Every array, record or cell has exactly one owner at every point in the program. The door works this out by itself. You write no `&'a mut`, no `Rc`, no `Arc`.

## Freed at last use

Stigi finds the last use of every object across all branches and frees or reuses its memory right there, not at the end of a scope. Objects that never leave a function live on its stack frame.

## Regions

Allocation happens in a region, passed as a capability:

```scala
// prompt: a zeroed buffer of the requested size.
def Demo.zeroed {region: Stigi.Region} (size: U64): Demo.Buffer =
  Array.tryFill{region}(size, 0) match
    case Filled(buffer) => Ready(size, buffer)
    case FillFailed => NoMemory
```

## Flat memory

Keeping a dead object alive while you keep allocating is a compile-time error, not a leak you find at 3 a.m. Long-running servers stay flat: zero major and zero minor page faults in steady state.

## Growing and trees

`Array.tryGrow` gives you a bigger array and consumes the old one, so no stale alias survives. Trees live in flat node tables with proved `below(child, count)` indexes, or as boxed linear trees updated in place with constant stack.

Next: [capabilities and handlers](/language/capabilities/).
