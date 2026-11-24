---
title: How Stigi compares
description: Stigi next to the runtimes people use today.
sidebar:
  order: 4
---

Each of these systems does something well, and we learned from all of them. The table only asks four questions: where does authority live, are resources bounded, can code die halfway through an effect, and is replay complete?

| Runtime | Authority | Resources | Death mid-effect | Replay and fork |
|---|---|---|---|---|
| **Docker / Firecracker** | OS sandbox at run time | Static caps, OOM kill on breach | Yes, SIGKILL | VM snapshots only |
| **WASI / Wasm components** | Import list at run time | Fuel and memory traps | Yes, traps | No |
| **Temporal / Restate + Wasm** | Host bindings at run time | Timeouts and limits | Yes, traps or OOM | Honor-system replay |
| **Linux eBPF** | Verified helper allowlist | Static stack, verifier cap | No resource protocols | No |
| **Erlang / BEAM** | Ambient I/O | Reductions, per-process heap | "Let it crash" leaves outside protocols open | No |
| **Unison** | Abilities in types | Garbage collected | No stop-state proofs | Partial |
| **Stigi** | **Proved at admission** | **Proved stack, cost and heap, with safe overcommit** | **No: proved stop states, tested failure arms** | **Complete by construction** |

Stigi also uses Wasm itself: when a Stigi module is exported as a Wasm component, its capability list becomes the component's import list. The sandbox then enforces what the door already proved.

For more projects we learned from, see [related work](/related/).
