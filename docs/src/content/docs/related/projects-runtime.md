---
title: "Projects: runtimes and systems"
description: Runtimes, sandboxes and systems near Stigi.
sidebar:
  order: 4
---

Stigi compiles to portable C and cares about safe, bounded, replayable execution. These runtimes and systems solve parts of that problem, and Stigi learns from each.

## Sandboxes and WebAssembly

- [Cloudflare Workers](https://workers.cloudflare.com/) — edge serverless runtime. Small isolated units, enforced at run time; Stigi proves the same limits at admission.
- [Docker](https://www.docker.com/) — container runtime. Stigi binaries run inside containers too, but do not need one for safety.
- [Firecracker](https://firecracker-microvm.github.io/) — microVM monitor. Strong isolation by virtualization.
- [gVisor](https://gvisor.dev/) — user-space sandbox kernel. A small trusted base between code and host.
- [Kubernetes](https://kubernetes.io/) — container orchestrator. A scheduler that can use Stigi's proved envelopes.
- [WASI](https://wasi.dev/) — WebAssembly system interface. Capability-based system calls, like Stigi's capabilities.
- [Wasmtime](https://wasmtime.dev/) — WebAssembly runtime. Runs Stigi modules exported as Wasm components.
- [WebAssembly](https://webassembly.org/) — portable bytecode sandbox. A Stigi target, where the capability list becomes the import list.

## Durable execution and replay

- [Antithesis](https://antithesis.com/) — deterministic simulation testing. The value of deterministic replay, at the VM level.
- [Determinator](https://duckduckgo.com/?q=Determinator+deterministic+operating+system) — deterministic operating system. Deterministic runs by construction.
- [FoundationDB](https://www.foundationdb.org/) — distributed database tested by simulation. Simulation-first testing.
- [Hydro](https://hydro.run/) — distributed programming framework in Rust. A typed view of distribution.
- [Restate](https://restate.dev/) — durable execution engine. Stigi makes the journal complete by construction.
- [Temporal](https://temporal.io/) — durable workflow engine. Stigi replaces honor-system determinism with capabilities.

## Kernels, VMs and verified systems

- [BEAM](https://www.erlang.org/) — Erlang virtual machine. Lightweight processes and per-process heaps.
- [eBPF](https://ebpf.io/) — verified in-kernel bytecode. Checked code in a small runtime.
- [HVM](https://github.com/HigherOrderCo/HVM) — interaction-net runtime. Parallel evaluation by default.
- [io_uring](https://kernel.dk/io_uring.pdf) — Linux asynchronous I/O. Stigi models its operations as linear tickets.
- [MirageOS](https://mirage.io/) — OCaml library operating system. Small unikernels with only what they need.
- [seL4](https://sel4.systems/) — verified microkernel. Proved capability isolation between processes.
- [Vor](https://github.com/vorlang/vor) — BEAM protocol checker for agents. Checked agent protocols.

## GPUs and compilers

- [CUTLASS and CuTe](https://github.com/NVIDIA/cutlass) — GPU matrix templates and layout algebra. Tiling and layout ideas for GPU kernels.
- [MLIR](https://mlir.llvm.org/) — compiler infrastructure. Layered intermediate forms.
- [XLA](https://openxla.org/xla) — machine learning graph compiler. Graph lowering for accelerators.

## Building blocks and craft

- [BLAKE3](https://github.com/BLAKE3-team/BLAKE3) — fast hash function. Used for content hashes.
- [Bun](https://bun.sh/) — JavaScript runtime. Attention to startup speed.
- [Cap'n Proto](https://capnproto.org/) — serialization system. Zero-copy data layouts.
- [Ghostty](https://ghostty.org/) — terminal emulator in Zig. Attention to speed.
- [RQlite](https://rqlite.io/) — distributed SQLite database. A small replicated store.
- [SQLite](https://www.sqlite.org/) — embedded SQL database. A small, very well tested core.
- [TigerBeetle](https://tigerbeetle.com/) — financial database in Zig. Determinism and static allocation.
