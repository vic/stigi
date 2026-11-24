---
title: Reviewing theory diffs
description: Review what changed in meaning, not 800 lines of text.
sidebar:
  order: 3
---

An agent refactors a subsystem across 25 units. The text diff is 800 lines. Nobody reads it, and somebody clicks merge.

Stigi's `diff` compares theory sheets instead:

```console
$ stigi diff --from prev --to head Payments.Worker
semantic delta across 25 edited units:
  authority:     UNCHANGED
  protocols:     UNCHANGED
  info-flow:     UNCHANGED
  laws:          2/2 PROVED, 0 weakened
  trust surface: UNCHANGED (0 new externs, 0 new wrapping ops)
  envelope:      time 22*n+90 -> 14*n+64 (-36%), stack 1840 -> 1408 B (-23%)
  shaken slop:   4 dead helpers removed, 11 constant folds
  refactors:     19 units pure extract/inline invariant (0 semantic change)
```

Zero changes in authority, protocols, flow and laws, and the code got faster and smaller. You can review that with confidence in ten seconds.

Now add one new outbound host, one wrapping arithmetic shortcut, or one flow from a secret key into a log. It shows up on the first lines, and you know exactly where to look.

Next: [testing is not running untrusted code](/humans/testing/).
