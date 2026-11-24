---
title: Checked docs
description: Explanations that fail when they stop being true.
sidebar:
  order: 2
---

A docstring can lie. An AI summary can lie even better. Stigi lets prose carry claims that the door checks.

## Claims with anchors

When someone, human or agent, explains a module, each factual sentence carries an `@claim(...)` anchor that points at a proved fact:

```text
[2. CHECKED SUMMARY — CLAIM-ANCHORED]
  [PROVED]  Never opens network connections outside ledger.internal:443.   (@claim manifest)
  [PROVED]  Every ledger write ends in Committed or RolledBack on all paths (@claim protocol)
            including FillFailed and socket disconnects.
  [TESTED]  Matches pure reference model Spec.Ledger.applyBatch across 10,000 seeded boundary cases.
  [UNCHECKED PROSE] Implements idempotent end-of-day settlement for EU merchant accounts.
```

If the text says "never touches the network" and that is false, the anchor fails with `claim-false`. If a later change makes it false, it fails with `explanation-stale`. Any sentence without an anchor is stamped `[UNCHECKED PROSE]`, so prose can never pass for proof.

## Decision tables

Public functions show their logic as a complete table of conditions and outcomes, built from their pattern matches. No nested `if` trees to read:

```text
Payments.classifyTransfer(amount, dailyUsed, limit)
  | condition                                  | outcome         |
  |--------------------------------------------|-----------------|
  | amount == 0                                | RejectZero      |
  | amount > 0 && dailyUsed + amount <= limit  | Approve(amount) |
  | amount > 0 && dailyUsed + amount > limit   | HoldForReview   |
```

When a table is too big to read, Stigi says `table-too-large` instead of hiding rows.

## Discovered laws

`stigi discover` tries small expressions over a public API and proposes laws like `decode(encode(x)) == Ok(x)`. They are labeled **TESTED** until a human promotes them to real laws.

Next: [reviewing theory diffs](/humans/theory-diff/).
