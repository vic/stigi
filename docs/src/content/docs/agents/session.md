---
title: An agent session
description: What it looks like when an agent works on a Stigi image.
sidebar:
  order: 1
---

Agents talk to a live Stigi image over a local socket or MCP. No shell, no file reads, no `grep`. Here is a whole task, start to finish.

## 1. Start with a stable prefix

The session begins with a prefix that never changes between turns: a short agent reference, the table of lessons and a zoomed-out view of the code. Because it is byte-stable, the model's prompt cache keeps it cheap.

```console
$ stigi zoom Billing L0
# about 110 tokens: capabilities, protocols, laws, invariants, envelopes
```

## 2. Ask for exactly what you need

```console
$ stigi slice Billing.Invoice.applyDiscount --depth 1
```

This returns the function, plus only the signatures and facts of its direct callees, callers, enums and the laws that touch it. Always in the same order.

## 3. Send a small edit

```console
$ stigi batch
alias Inv = Billing.Invoice
edit Inv.applyDiscount.arm[Percentage(rate)] :=
  case Percentage(rate) if rate <= 100 => Inv.scaleDown(total, rate)
```

The edit names a path inside the unit, so the agent writes only what changes. The `alias` lives only on the wire; the stored code keeps full names.

## 4. Read one clear answer

```text
rejected {Billing.Invoice.applyDiscount}: precondition-unproved (1 root, 3 downstream collapsed)
  need: below(rate, 101) at Billing.Invoice.applyDiscount.arm[Percentage]
  counterexample: total=500, rate=101 -> violates Inv.scaleDown entry fact
  fix1 (checked): refine Percentage field with <lessOrEqual(rate, 100)>
  lesson: precondition-unproved (see cached prefix)
```

## 5. Accept the fix

```console
$ stigi accept fix1
admitted Billing.Invoice.applyDiscount   laws ok   tests 42/42   shaken: 0
```

Next: [why tokens matter](/agents/tokens/).
