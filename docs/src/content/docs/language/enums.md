---
title: Enums and pattern matching
description: Closed enums, exhaustive matches, and cases with contracts.
sidebar:
  order: 5
---

An `enum` lists every shape a value can take. Nothing else can exist.

```scala
// prompt: the result of looking up a key.
enum Demo.Lookup:
  case Found(slot: U64)
  case Missing
```

## Matching

A `match` must name every case. A catch-all `_` over a closed enum is rejected, so adding a new case later shows you every place that must handle it.

```scala
// prompt: the slot of a lookup, or zero when missing.
def Demo.slotOrZero(lookup: Demo.Lookup): U64 =
  lookup match
    case Found(slot) => slot
    case Missing => 0
```

Patterns nest as deep as you like. A missing combination is reported with an example, and an arm that can never run is reported too.

## Several heads

A function can be written as a list of cases, like in Erlang:

```scala
// prompt: combine two lookups.
def Demo.combine(left: Demo.Lookup, right: Demo.Lookup): U64 =
  case (Found(a), Found(b)) if a < 1000 && b < 1000 => a + b
  case (Found(a), Missing) => a
  case (Missing, Found(b)) => b
  case (Missing, Missing) => 0
```

## Cases with a contract

A case can carry facts. They are checked when the value is built and given back when it is matched:

```scala
// prompt: a position inside the variables of one rule.
enum Demo.Position:
  case Inside(variables: U64, position: U64) <below(position, variables)>
  case Outside
```

Only code in the enum's own namespace can build these values. Others can read and match them, but never forge them.

Next: [errors are values](/language/errors/).
