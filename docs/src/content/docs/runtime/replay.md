---
title: Replay and fork
description: Byte-identical replay of agent runs, without a hypervisor.
sidebar:
  order: 3
---

Workflow engines ask you to follow determinism rules on the honor system. Call the clock, read an environment variable or iterate an unordered map, and replay breaks in production. Simulation tools that record a whole Linux VM fix this, at a big cost in compute and startup time.

Stigi has no ambient authority. The LLM, the clock, the file system, the network and the scheduler are all capabilities. So recording what crosses those capabilities captures **all** the outside nondeterminism, by construction.

## Replay

Swap the live handlers for a journal-replay handler and run the agent again, in process, at native C speed. Thousands of runs replay byte for byte, with zero nondeterminism errors.

## Fork at step k

Fork a session at step `k`: inject a tool timeout, a smaller memory limit or a different LLM answer. Effects before step `k` come from the journal and never run again. Calls after step `k` branch from explicit, versioned state. No memory snapshots, no microVM.

## What replay is, exactly

Replay reproduces what was **recorded**. It does not bring back an outside world that moved on after step `k`, and a live LLM call is still nondeterministic: it is recorded, not predicted.

Next: [how Stigi compares](/runtime/comparison/).
