---
title: Facts in angle brackets
description: How Stigi replaces runtime checks with proved facts.
sidebar:
  order: 3
---

A **fact** is a small statement the door can prove, written in `<...>`. Two you will see everywhere:

- `below(index, count)` means `index < count`.
- `lessOrEqual(a, b)` means `a <= b`.

## Entry facts and exit facts

Facts before the `:` are **entry facts**. The caller must prove them. Facts after the result type are **exit facts**. The body must prove them, and the caller gets them for free.

```scala
// prompt: the element at a proved index.
def Demo.Table.at [A: Type] (count: U64, table: Array[A](count), index: U64) <below(index, count)>: A =
  table(index)
```

Inside the body, `below(index, count)` is known, so `table(index)` needs no check. Every caller has to show the index is in range, by a loop guard, an `if` or a fact it got from somewhere else.

## Facts replace ifs

If the facts already decide a condition, Stigi rejects the `if` as `branch-decided`. A defensive check that can never fail is noise, and it costs time at run time. Facts carry the knowledge instead.

## Facts at a point

`@<...>` states a fact at a point in the code, and the door proves it there. `expression@<...>` states a fact about one value:

```scala
val position = Demo.find(size, block, target)@<lessOrEqual(self, size)>
```

Next: [loops and invariants](/language/loops/).
