---
title: The proved runtime envelope
description: Four runtime layers, replaced by one admission verdict.
sidebar:
  order: 1
---

If you run AI agents, tool runtimes, serverless workers or RL rollouts, this is the part every other stack misses.

## Four layers today

Runtimes treat agent-written code as a black box, so they wrap it in four separate layers:

1. **Sandboxes**: Docker, Firecracker, Wasm traps, seccomp and cgroups.
2. **Headroom and timeouts**: extra memory just in case, and OOM kills when it is not enough.
3. **Partial traces**: best-effort logs and replay that breaks when code touches a clock or a file.
4. **Manual permission review**: people reading dependencies to guess what code might touch.

Worse, they fight each other. An OOM kill or an out-of-fuel trap halfway through a tool call leaves locks, files and sockets torn.

## One verdict, one envelope

In Stigi, the same verdict that admits a unit produces its **runtime envelope**:

1. **Narrowed authority**: the exact [manifest](/door/authority/) of files, hosts, clocks and C bindings.
2. **No death mid-effect**: failures are values and every protocol reaches a stop state on every path, so Stigi code never traps halfway through an effect.
3. **Resource bounds**: proved stack bytes, step counts and heap as a formula of the input size.
4. **A complete effect journal**: every outside call, LLM calls included, goes through a capability, so it can be recorded and replayed.

```text
unit:       Tools.JsonSchema.validatePayload
authority:  none (pure; Region only)
stack:      640 bytes (proved, signal reserve included)
time:       14 * inputBytes + 32 ops   [O(n)]
heap:       2 * inputBytes + 128 bytes [flat; 0 retained dead bytes]
failures:   FillFailed (tested), InvalidUtf8 (tested), SchemaMismatch (tested)
protocols:  none open on any exit path
```

That record is what a scheduler receives. Next: [scheduling with it](/runtime/scheduling/).
