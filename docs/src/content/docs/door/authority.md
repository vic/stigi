---
title: No ambient authority
description: A proved list of everything a program can touch.
sidebar:
  order: 5
---

Supply-chain attacks and rogue tool calls work because almost every language hands every package full power. Any helper can read environment variables, open sockets, touch files or run scripts at build time.

In Stigi there is no ambient I/O, at run time or at build time. No build scripts, no hidden globals, no unchecked system calls. A unit can do an effect only with a [capability](/language/capabilities/) its caller handed to it.

## The manifest

Because every effect is a capability, Stigi can tell you exactly what a program may touch:

```console
$ stigi manifest Billing.Service.main
root Billing.Service.main:
  files:   read-only under ./config
  network: outbound TLS to payments.internal:443
  clock:   monotonic only
  shell:   none
  externs: 2 counted C bindings (see `trust`)
```

This is not a guess from reading the code. It is proved by the door, and it changes only when the code changes.

## C bindings are counted

Calls into C are the one place where Stigi trusts code it did not check. So every `extern "c"` binding is listed and counted. `stigi trust` shows them all, together with any user rules and assumptions a proof relies on.

Next: [checked complexity](/door/complexity/).
