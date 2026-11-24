---
title: Search by contract
description: Find a function by what it promises, not by what it is called.
sidebar:
  order: 7
---

Grepping for names works when you already know the name. Often you only know what you need: these inputs, this result, no effects, fast enough.

Stigi lets you search the image by the whole contract: parameter types (in any order), entry and exit facts, capabilities and proved cost. `?` stands for "anything":

```console
$ stigi search "(size: U64, block: Array[U8](size), target: U8) -> ? {pure} <cost <= O(log size)>"
1. Search.binaryFind : (size: U64, block: Array[U8](size), target: U8) <lessOrEqual(1, size), cost(O(log size))> -> Search.Lookup
   proved bound: 9 * log2(size) + 4 steps, 0 heap allocs, 48 B stack
2. Search.lowerBound : (size: U64, block: Array[U8](size), target: U8) <cost(O(log size))> -> U64 <lessOrEqual(result, size)>
   proved bound: 8 * log2(size) + 3 steps, 0 heap allocs, 48 B stack
```

## Proved, not claimed

Search reads the costs and facts the door proved. It never reads comments or annotations nobody checked. An agent cannot fake its way into a search result by writing a nice docstring.

Results come in a fixed, deterministic order, so the same query always gives the same answer, byte for byte.

Next: [checked synthesis](/door/synth/).
