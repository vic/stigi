---
title: Checked synthesis
description: Holes in your code, filled with candidates the door already checked.
sidebar:
  order: 8
---

Sometimes you know the shape of what you need, but not the exact call. Leave a hole, `?(name)`, and ask Stigi to fill it.

```console
$ stigi synth Demo.Router.dispatch ?(handler) --budget O(log count)
candidate1 (checked, cost 11 * log2(count) + 6):
  Search.binaryFind(count, routes, key) match
    case FoundAt(slot) => DispatchHit(slot)
    case Missing       => DispatchMiss
```

## How it works

Stigi looks at the type the hole expects, every fact in scope, the capabilities you hold and the cost budget you gave. It drops every library function that costs too much, then searches for short answers: a chain of facts, a law to call, a `match` over every case, or a few calls glued together. Every candidate goes through the door before you see it.

Synthesis is good at the small, fiddly glue: up to a few dozen syntax nodes. New algorithms are still the job of people and language models, checked by [the law loop](/door/law-loop/).

## No hidden macros

Holes live only in dry-run checks. A unit with a hole cannot be admitted. When you accept a candidate, it becomes plain, readable Stigi source that you can read, diff and keep in git. Nothing expands behind your back at compile time.

Next: [the law loop](/door/law-loop/).
