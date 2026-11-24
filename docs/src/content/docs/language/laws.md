---
title: Laws
description: Theorems checked for every input, not three examples.
sidebar:
  order: 11
---

A test checks a few examples. A **law** holds for every input. In Stigi a law is a `lemma def`:

```scala
// prompt: narrowing a proved offset inside a slice stays strictly inside the outer buffer.
pub lemma def Slice.offsetInsideOuter(outerLength: U64, sliceStart: U64, sliceLength: U64, offset: U64)
    <lessOrEqual(sliceStart + sliceLength, outerLength), below(offset, sliceLength)>:
    <below(sliceStart + offset, outerLength)> =
  ()
```

Read it as: if the slice fits in the buffer and the offset is inside the slice, then the offset is inside the buffer.

## What a law gives you

- Its entry facts are proved at each call, and its exit facts become known there.
- It has no runtime cost. The emitter erases it.
- It is checked again whenever anything it mentions changes. A change that breaks a law is rejected, and the law is named.

So an agent can rewrite a 400-line function to make it twice as fast, and the door shows it still obeys every law, or names the one it broke.

## Reference models

For properties beyond arithmetic and protocols, like sorting or compression, a human writes a tiny pure model, say `Spec.decode`, and the agent writes `Fast.decode`. Stigi runs seeded boundary tests that pair the two, erases `Spec.decode` from the binary, and reports the result as **TESTED**, never as proved.

`stigi laws` lists every law of a namespace, split into kernel logic, user definitions and trusted assumptions.

Next: [traits and givens](/language/traits/).
