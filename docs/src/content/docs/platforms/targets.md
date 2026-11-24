---
title: From microcontrollers to GPUs
description: One checked program, many hardware profiles.
sidebar:
  order: 3
---

Stigi keeps one checked representation of your program across very different machines. A target profile changes integer widths, byte order, stack budgets and the table of available C bindings. It never changes a safety rule.

- **Linux servers** on x86-64, aarch64 and riscv64. Memory pools are populated up front, or locked in RAM for real-time work.
- **Embedded boards** with no libc at all. Only device capabilities, and tight proved stack budgets.
- **WebAssembly components.** The capability list of a module is its Wasm import list, so the sandbox and the door agree.
- **GPU kernels** over integers. Tiles are proved not to overlap, and barriers are part of the type, so shared-memory data races are rejected at compile time.

The same rules that keep a server process safe keep a toaster safe.

Next: [trust](/trust/self-hosting/).
