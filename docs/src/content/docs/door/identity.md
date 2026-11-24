---
title: Hashes and dependencies
description: Every unit is known by a hash of what it computes, and the door re-checks only what a change can reach.
sidebar:
  order: 1.5
---

Every unit in the image has an id: a BLAKE3 hash of its checked code. Names are not in it. Two units that compute the same thing, with the same capabilities, have the same id, whatever you called them.

## What goes into a hash

A unit hashes what survives at run time: the structure of its computation, its runtime types and layouts, the capabilities it uses, the C symbols it calls, and the ids of every unit it references.

Names, comments, prompt lines and source positions are left out. So are facts, laws and termination measures, because they are checked at compile time and erased before the C code is written.

That gives you a few nice things:

- **Renaming is free.** Rename a function or a variable, and its id stays the same. Nothing downstream needs to be checked again.
- **Pure refactors are visible.** When a refactor does not change what a unit computes, the id shows it, and [theory diffs](/humans/theory-diff/) report zero semantic change.
- **Sharing code is safe.** You can move units between machines through content-addressed stores. The receiver never trusts the sender: every unit goes through the receiver's own door.

## A graph of ids

A unit hashes the ids of everything it references, so the whole image is a graph of ids with no cycles, like in Nix or Unison. Change one unit, and its id changes, and so do the ids of everything that uses it. Nothing else moves.

## What happens when code is admitted

When a batch reaches the door, the door builds a candidate image and runs every check on it: names, types, facts, Datalog rules, NegTests, Tests and closure. Then it either swaps in the new image or drops it. Nobody ever sees half of a change.

The door does not check the whole image again. It checks:

- the units in the batch,
- every unit that reaches a changed id through the graph,
- every unit whose facts a changed rule reads.

Everything else keeps its verdict. A verdict is keyed by the unit's checked obligations, its runtime id and the ids of the rules in force for it, so a cached verdict is never stale.

If a change would break a unit somewhere else, for example a rule change that one old unit no longer passes, the whole batch is rejected and the reply lists that unit. The image is always valid under its current rules.

Next: [one reasoning engine](/door/datalog/).
