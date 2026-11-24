---
title: Native C, both ways
description: Call C from Stigi, and ship Stigi as a C library.
sidebar:
  order: 1
---

## Stigi is C underneath

Stigi compiles to plain, portable C11. You build it with GCC or Clang, like any C. The emitted code is compiled with `-fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks`, so even a checker bug cannot turn into optimizer-exploited undefined behavior.

## Calling C

An `extern "c"` unit binds a C symbol:

```scala
// prompt: POSIX write; reads at most count bytes of the block.
extern "c" def write(descriptor: I32, block: Array[U8](count), count: U64): I64
```

C cannot be proved, so it is counted. A binding declares its stack cost and what it does to its buffers, it may only be used inside a handler, and it is listed in `stigi trust`.

## Shipping a C library

You do not have to rewrite your company in Stigi to get value on day one:

```console
$ stigi emit --export-c ./out/libtokenizer
```

That gives you a static or shared C library with a generated header. No panics, no OOM crashes, a bounded stack and no ambient I/O. Call it from Python, Rust, Go or C++.

Good first candidates are the small, hot, risky pieces of AI serving stacks: an LLM BPE tokenizer, or a JSON-schema mask engine for constrained decoding.

Next: [parallel by default](/platforms/parallelism/).
