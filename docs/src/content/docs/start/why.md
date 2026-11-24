---
title: Why Stigi exists
description: AI broke the old contract between people and the code they run.
sidebar:
  order: 1
---

I have no problem with AI writing code. The problem is living with AI-generated slop that nobody understands, nobody can run safely without a wall of containers around it, and nobody can prove does what it should.

:::note[Stigi is a work in progress]
Stigi is an ambitious project, and it is still growing. I build it in the open, on my own time and with my own money. If you want to see it happen sooner, [sponsoring it](https://github.com/sponsors/vic) is the most direct way to help. Trying it on a real problem and [telling us what breaks](https://github.com/vic/stigi/issues) helps just as much. Thank you :)
:::

## How things work today

Look at what happens when an agent writes your code:

1. **You pay twice for safety, and still get leaks.** The agent writes Python, TypeScript, Go or Rust. Then you wrap it in Docker, add extra RAM because nobody knows the real peak, set a timeout and hope an OOM kill does not leave a lock or a half-written file behind.
2. **Most of your token budget goes to searching.** Agents grep through folders, read 1,000-line files to find one signature, rewrite whole modules to change three lines, and read long compiler logs again and again.
3. **You review by running tests.** Nobody reads 600 lines of generated code. CI goes green and the change is merged, even when the agent mocked a handler or weakened a check.
4. **Even proofs got unreadable.** A machine can check millions of lines of AI-written proof, and still no human understands the result.

## What Stigi does instead

Every unit of code enters one live **image** through one strict **door**. Before a unit gets in:

- One Datalog engine checks ownership, bounds, arithmetic, stack depth, resource protocols, permissions, complexity and laws, all together.
- Unused code, unused variables and dead branches are removed, so slop never enters.
- A rejected edit comes back with the exact missing fact, a counterexample and a checked fix the agent can accept in two tokens.
- Humans work with laws, protocols, permission manifests and decision tables, not with walls of text.

The image, not a folder of text files, is the thing that lasts. From it, Stigi gives agents a cheap protocol, gives reviewers a short theory sheet, and gives production a proved runtime envelope.

Next, let's look at some code: [a two-minute tour](/start/tour/).
