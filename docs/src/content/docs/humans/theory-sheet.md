---
title: The proved theory sheet
description: One screen that tells you what a subsystem does, proved by the door.
sidebar:
  order: 1
---

This is the part that decides whether humans stay in control of AI-built software.

## Why diagrams do not help

When code grows past reading speed, people try to draw it as boxes and arrows. That does not scale. Human working memory breaks at around fifty things on screen. Turning 5,000 lines into 5,000 boxes simplifies nothing.

People are bad at holding ten thousand details. People are very good at abstract thinking: boundaries, state machines, invariants, tables and theorems. So the simplifying has to happen in the math, before anything is drawn.

## The sheet

`stigi abstract` turns any subsystem into a **theory sheet**. Its size grows with the ideas in the code, not with the lines:

```text
$ stigi abstract Payments.Worker.main
THEORY SHEET: Payments.Worker.main   (312 units collapsed, 0 ambient I/O)

[1. AUTHORITY, INVARIANTS & ENVELOPE — PROVED]
  Capabilities:  Files[write-temp-rename under ./ledger], Socket[TLS ledger.internal:443], Clock[mono]
  Trust surface: 2 counted C externs (posix_fsync, tls_write), 0 user axioms
  Invariants:    Cents(amount) >= 0; BatchSize <= 256; AccountIndex < accountCount
  Envelope:      stack <= 1,840 B | time <= 22*entries + 90 ops [O(n)] | heap <= 64*entries + 4 KiB

[3. EXERCISED CAPABILITY PROTOCOLS — PROVED]
  LedgerFile:  Init --[begin]--> Staging --[append*]--> Staging
               Staging --[commit]--> Committed (stop)
               Staging --[FillFailed | IoErr -> rollback]--> RolledBack (stop)
  Denials:     NEVER close(Socket) while LedgerFile is Staging

[4. LAWS — PROVED]
    - Ledger.balanceConserved : sum(after) == sum(before)
    - Ledger.feeBounded       : fee <= amount / 20
```

A full sheet has five views: authority and envelope, checked summary, protocols, laws with decision tables, and an information-flow matrix. Each is derived from facts the door already proved.

## Zooming out never lies

Like abstraction in static analysis, zooming out forgets details but never invents a guarantee. `stigi zoom` goes from this one screen down to the code, one level at a time.

Next: [checked docs](/humans/checked-docs/).
