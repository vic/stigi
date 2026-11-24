---
title: Traits, givens and more
description: Generic code with zero runtime cost, and the modern conveniences you expect.
sidebar:
  order: 12
---

## Traits and givens

A `trait` describes operations over a type. A `given` implements it for a concrete type.

```scala
// prompt: types whose values can be compared for equality.
trait Demo.Equal[A: Type]:
  def same(left: A, right: A): Bool

// prompt: equality of U64 values.
given Demo.EqualU64: Demo.Equal[U64]:
  def same(left: U64, right: U64): Bool = left == right
```

A function asks for an implementation with `{Demo.Equal[A]}`. The compiler finds the given at compile time and specializes the code. There is no runtime dictionary and no virtual call. If two givens match, you get `ambiguous-givens` instead of a silent pick.

## Pipes

`x |> f(a, b)` means `f(x, a, b)`. It keeps chains of calls readable:

```scala
Demo.parse(size, text) |> Demo.validate |> Demo.exitStatus
```

## And the rest

Stigi also has lambdas (compiled to plain functions with an exact call graph), effect handlers that resume more than once where you need them, 128-bit integers, deterministic floating point reductions, generation-checked slot tables that catch stale indexes, and fixed-layout records with explicit byte order. All of them go through the same door and compile to flat C.

Next: [what Stigi leaves out](/language/left-out/).
