---
title: One reasoning engine
description: Why every check in Stigi is a Datalog fact.
sidebar:
  order: 2
---

In most ecosystems safety is spread over tools that do not talk to each other. A type checker here, a borrow checker there, a linter, a static analyzer nobody reads, a sanitizer, a container. Each one sees a slice, so each one stays careful, and gaps open between them.

In Stigi, memory, layout, purity, bounds, arithmetic, protocols, permissions, complexity and dead code are all **facts in one Datalog engine**. Every check sees what every other check proved.

## Why Datalog

Datalog is a small logic language: facts, and rules that derive new facts from old ones. Stratified Datalog always finishes, always gives the same answer, and runs in polynomial time. There is no SMT solver that times out, no search that depends on luck.

A rule reads like this:

```scala
// prompt: below passes through lessOrEqual on its right side.
rule def Stigi.Bounds.belowThenLessOrEqual: Stigi.Facts.below(low, high) =
  Stigi.Facts.below(low, middle), Stigi.Facts.lessOrEqual(middle, high)
```

If `low < middle` and `middle <= high`, then `low < high`. Rules are units too, with names, prompts and edges. Changing a rule checks again every unit that depends on it.

## Exact numbers

Facts about numbers use exact 128-bit arithmetic, and overflow fails closed. The checker never rounds and never guesses.

Next: [shaking out slop](/door/slop/).
