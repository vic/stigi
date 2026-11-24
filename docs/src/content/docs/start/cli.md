---
title: The stigi command
description: The handful of commands you use every day.
sidebar:
  order: 3
---

You talk to a Stigi image with one command, `stigi`. Here are the ones you will use most.

:::note[Stigi is a work in progress]
Stigi is an ambitious project, and it is still growing. I build it in the open, on my own time and with my own money. If you want to see it happen sooner, [sponsoring it](https://github.com/sponsors/vic) is the most direct way to help. Trying it on a real problem and [telling us what breaks](https://github.com/vic/stigi/issues) helps just as much. Thank you :)
:::

## Load and emit

```console
$ stigi load ./tree
$ stigi emit ./out/app.c
$ stigi emit --export-c ./out/libtokenizer
```

`load` reads a reviewed source tree into an empty image, through the door. `emit` writes verified C11. With `--export-c` you get a C library and its header, ready to call from any language that speaks C.

## Understand

```console
$ stigi abstract App.Main
$ stigi manifest Billing.Service.main
$ stigi diff --from prev --to head
```

`abstract` prints the [theory sheet](/humans/theory-sheet/) of a root or a namespace. `manifest` prints everything a root is allowed to touch. `diff` compares two versions by meaning: authority, protocols, laws, flow and envelopes.

## Find and edit

```console
$ stigi search "(count: U64, bytes: Array[U8](count)) -> ? {pure} <cost <= O(count)>"
$ stigi slice App.Parser.parseHeader --depth 1
$ stigi synth Demo.Router.dispatch ?(handler) --budget O(log count)
```

`search` queries the image by a whole contract. `slice` gives an agent the exact context it needs for one edit. `synth` proposes checked code for a hole.

Agents use the same image over a local socket or MCP. The [agent session](/agents/session/) page shows how that looks.
