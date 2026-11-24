---
title: Tokens are money
description: Where an agent's budget really goes, and what Stigi does about it.
sidebar:
  order: 2
---

When AI writes your software, tokens are your cost of goods. Yet every mainstream language was built for people typing into files and reading scrolling logs.

## Where the tokens go

Across a real multi-turn task, most of the bill goes to:

1. **Finding context**: grepping and reading whole files to find three signatures.
2. **Rewriting whole files**: 400 lines of output to fix a 3-line bug.
3. **Retry loops**: counting string bytes by hand, guessing missing preconditions, chasing cascades of errors.
4. **Long diagnostics**: the same walls of compiler text, every turn.
5. **Broken prompt caches**: timestamps and shifting order that throw away cheap cached reads.

## Why we do not invent a terse syntax

Some "AI languages" try to save tokens with glyphs, one-letter names or raw JSON trees. Research like [SimPy](https://arxiv.org/abs/2404.16333) and [Token Sugar](https://arxiv.org/abs/2512.08266) shows the ceiling of that is around 10 to 13 percent per file, and it breaks what the model learned in pretraining. The model thinks more and retries more, so the total bill goes up.

## What Stigi compresses instead

The image does not depend on text syntax. So Stigi compresses everything the door **sends**, and keeps plain Scala-like text for everything the agent **writes**:

- Full-word names and `// prompt:` lines stay. Common words are single tokens anyway.
- String lengths are inferred, so nobody counts bytes.
- Edits name a path inside a unit, so output scales with the change.
- Reads, queries and errors use compact columnar tables, byte-stable for the prompt cache.
- Cascades of errors collapse to one root cause.

## Measured like speed

Every compiler change is measured on tokens-to-green (cached input, fresh input and output), on how often the same rejection repeats, and on the cache-hit rate. A change that makes agents spend more does not land.

Next: [rejections that teach](/agents/rejections/).
