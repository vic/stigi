---
title: Errors are values
description: No exceptions, no null, no panics.
sidebar:
  order: 6
---

Stigi has no exceptions, no `null` and no panics. When something can fail, the result says so with an enum case, and the caller handles it.

```scala
// prompt: the outcome of parsing a port number.
enum Demo.Port:
  case Valid(number: U64) <below(number, 65536)>
  case TooLarge
  case Empty
```

## Running out of memory

Allocation can fail too, and that is just another value:

```scala
val Filled(buffer) = Array.tryFill{region}(size, 0) else case FillFailed =>
  OutOfMemory
```

`val Filled(buffer) = ... else case FillFailed => ...` binds the good case and handles the bad one right there, without nesting.

## No crash paths, anywhere

There is no `abort`, `exit`, `unwrap` or panic in Stigi code, the standard library included. Failures travel as values all the way up to `main`, which turns them into an exit status. The emitted C contains zero `abort(` and zero `exit(` calls.

## Every failure arm is tested

A failure handler that never ran is a bug waiting to happen. Stigi's test runner injects `FillFailed` and other failures on purpose, and a failure arm that no test exercises is rejected.

Next: [memory without a garbage collector](/language/memory/).
