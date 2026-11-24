---
title: Code identity
description: Units are known by what they compute, not by their names.
sidebar:
  order: 4
---

Every Stigi unit has a hash. The hash comes from the unit's checked computation after names and laws are erased, and from the capabilities it needs at run time. It does not come from the text.

That gives you a few nice things:

- **Renaming is free.** Rename a function or a variable, and its identity stays the same. Nothing downstream needs to check again.
- **Pure refactors are visible.** When a refactor does not change what a unit computes, the hash shows it, and [theory diffs](/humans/theory-diff/) report it as zero semantic change.
- **Sharing code is safe.** You can move units between peers through content-addressed stores. The receiver never trusts the sender's claim: every unit goes through the receiver's own door.

Next: [trust](/trust/self-hosting/).
