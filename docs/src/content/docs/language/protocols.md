---
title: Resource protocols
description: Files, statements and sockets that are always used in the right order.
sidebar:
  order: 9
---

A file must be open before you read it, and closed before you are done. Most languages leave that to discipline. Stigi makes it part of the capability.

```scala
// prompt: one open file; it must be closed on every path.
capability Demo.File:
  state Open, Closed
  start Open
  stop Closed
  def read <at(self, Open)> (count: U64, block: Array[U8](count)): Demo.File.Read <at(self, Open)>
  def close <at(self, Open)> (): Demo.File.Close <at(self, Closed)>
```

`at(file, Open)` is a fact like any other. The door proves that:

- every operation runs only in a state that allows it,
- all branches agree on the state when they meet again,
- every handle reaches a `stop` state on every way out, errors included,
- two aliases of one stateful handle are never passed to the same call.

## Rules across resources

You can also state ordering rules between resources:

```scala
// prompt: count rows while proving the statement finishes before the database closes.
pub def Billing.countRows {database: Billing.Database} <at(database, Open)>
    (size: U64, queryText: Array[U8](size)): U64 <at(database, Closed)> =
  handle statement = Billing.Statement(database, size, queryText)
  @<not (close(database), at(statement, Prepared))>
  val rows = statement.stepCount()
  statement.finish()
  database.close()
  rows
```

If someone later moves `database.close()` above `statement.finish()`, the door rejects the change.

## A note on atomicity

Reaching `Closed` means the protocol finished cleanly. It does not by itself make raw writes crash-safe. When you need that, model it in the protocol, for example write-to-temp-then-rename with stop states `Committed | RolledBack`.

Next: [tests and NegTests](/language/tests/).
