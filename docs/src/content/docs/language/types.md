---
title: Numbers, arrays and types
description: The basic types, and why an array knows its own length.
sidebar:
  order: 2
---

## Numbers

Stigi has fixed-size integers: `U8`, `U32`, `U64`, `I32`, `I64` and `U128`. Indexes and lengths are always `U64`. There is no implicit conversion. Widening is a named function, and narrowing needs a proof:

```scala
val wide: U64 = U64.ofU8(byte)
val small: U8 = U8.ofU64(value)   // only where below(value, 256) is proved
```

`Bool`, `Unit` (written `()`) and `Never` complete the basics.

## Arrays carry their length

An array type says exactly how many elements it holds:

```scala
// prompt: the first byte of a non-empty block.
def Demo.first(size: U64, block: Array[U8](size)) <below(0, size)>: U8 =
  block(0)
```

Because `size` is part of the type, the door knows the bounds of every read. That is where the missing bounds checks go: they are proved, so the C code never needs them.

A text literal `"abc"` is an `Array[U8](3)`. You do not need to count bytes yourself; write `Array[U8](_)` and Stigi fills in the length.

## Types with a contract

A `type` can carry facts. Then a value converts to it only where the facts are proved:

```scala
// prompt: a decimal digit.
type Demo.Digit = U64 <below(self, 10)>
```

Once you hold a `Demo.Digit`, you can use `below(digit, 10)` anywhere without checking it again.

Next: [facts in angle brackets](/language/facts/).
