---
title: Rejections that teach
description: Every "no" from the door says why, shows a case, and offers a checked fix.
sidebar:
  order: 3
---

A compiler error is usually where an agent starts guessing. In Stigi, every rejection is built to end the guessing in one turn.

## What a rejection carries

```text
rejected {Demo.Table.peek}: index-unproved at Demo.Table.peek.body[0]
  need: below(index, count)
  fix1 (checked): add entry fact <below(index, count)>
  lesson: index-unproved (see cached prefix)
```

- **A code and a place.** `index-unproved`, at an exact path inside the unit.
- **The missing fact.** Not "something is wrong", but the one fact that would make it right.
- **A counterexample**, when a property is false: concrete values that break it.
- **Checked fixes.** Each `fixN` already passed the door. Accepting one costs two tokens.
- **A lesson pointer.** Each code has an example in the image's own NegTests. The table of lessons sits in the cached prefix, so the reply only points at it.

## One root, not fifty errors

When one missing fact breaks ten call sites, you get one root cause and a count of the sites it collapses. Fix the root and the rest go away.

## Fixes never hide work

The door applies only cleanups that change nothing about behavior, like removing unused binders. It never slips a runtime guard into your code. A guard, an invariant or an entry fact is always something you accept on purpose.

Next, the human side: [the proved theory sheet](/humans/theory-sheet/).
