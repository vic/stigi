---
title: Tests and NegTests
description: Tests that run at compile time, and tests that prove the checker rejects bad code.
sidebar:
  order: 10
---

## A Test

A Test is a `val` with the capability `{Stigi.Test}`:

```scala
// prompt: larger gives the right operand when it is greater.
val Demo.Tests.largerRight {Stigi.Test}: Stigi.Test.Result =
  Stigi.Test.expect(Demo.larger(2, 5), 5, "the right operand is greater")
```

`expect` takes what you got, what you want and a reason. Tests run inside the door, at compile time, with no file system, no network and no clock. Any capability a Test needs is handled by a local, deterministic handler.

## A NegTest

A NegTest shows that bad code is rejected, and for the right reason:

```scala
// prompt: an index with no bound is rejected.
val Demo.NegTests.indexWithoutBound {Stigi.NegTest}: Never =
  def at(count: U64, table: Array[U64](count), index: U64): U64 = table(index)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "no fact bounds the index")
```

It passes only when the local code is rejected with exactly that code. NegTests are how Stigi documents its own rules: each rule has the code it rejects, and the example that triggers it.

## Tests that cannot cheat

- `expect(true, true)` and `expect(f(1), f(1))` are rejected as tautologies.
- A test that touches a failure arm and ignores its result does not count.
- Tests are never `pub`, and nothing in production can call them.

Tests in Stigi check behavior. Safety is never left to tests: it is [proved](/humans/testing/).

Next: [laws](/language/laws/).
