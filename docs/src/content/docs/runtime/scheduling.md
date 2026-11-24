---
title: Exact packing and safe overcommit
description: Two ways to schedule Stigi workloads, and neither one kills a worker.
sidebar:
  order: 2
---

A common worry about static bounds: they are pessimistic. An agent loop may allow 50 steps and use 3 on average. A model may allow 8,192 tokens and write 400. Packing everyone for the worst case wastes 10 to 50 times the memory.

Stigi handles both kinds of workload.

## Mode 1: exact packing

For tools whose input size is known when the request arrives (parsers, tokenizers, JSON-schema masks, protocol decoders, bounded tool calls), the scheduler plugs the real size `n` into the proved formula `(heap(n), stack, ops(n))`. Tenants pack tightly, with no guessing.

## Mode 2: safe overcommit

For bursty work (LLM output streams, multi-step agent loops), the scheduler packs by typical use. If a rare burst hits the memory cap, Stigi does **not** kill the worker. The allocation returns `FillFailed`, a failure handler that tests already exercised runs, every open file, socket and transaction walks to its stop state, and control goes back to the scheduler.

This is the edge over Docker and Wasm: overcommit without torn state.

## Where a cap is still needed

Code with no static bound still needs a runtime cap. Stigi tells you which units those are: their cost is reported as unproved.

Next: [replay and fork](/runtime/replay/).
