---
title: The image and the door
description: Where Stigi code lives, and the one way in.
sidebar:
  order: 1
---

## The image

A Stigi program does not live as a pile of text files. It lives in an **image**: a content-addressed database of checked units, their facts and the edges between them.

Text is still there. You keep a reviewed source tree in git or jj, one unit per file, and `stigi load ./tree` replays it into an empty image. But the thing Stigi trusts is the checked image, not the text.

## The door

Every change to the image goes through one **door**. The door takes a whole batch of units and either accepts all of it or none of it. On the way in, each candidate goes through:

1. parse,
2. names and visibility,
3. types,
4. facts and Datalog rules,
5. NegTests and Tests,
6. closure and tree shaking.

There is no back door. Not for the standard library, not for tests, not "just for now". Loading a tree from disk runs the same door, unit by unit.

## Two ways in, two kinds of authority

Agents send small edits to a live image. Humans change the reviewed tree and load it. Both go through the same door, but only the reviewed tree can change laws, tests and public contracts. More on that in [the law loop](/door/law-loop/).

Next: [hashes and dependencies](/door/identity/).
