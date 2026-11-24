---
title: Capabilities and handlers
description: How effects enter a program, and only where you allow them.
sidebar:
  order: 8
---

In most languages any function can read the environment, open a socket or write a file. In Stigi, a function can do an effect only when its caller hands it a **capability**. Capabilities live in `{...}` in the signature.

```scala
// prompt: print a greeting to the console.
def Demo.greet {console: Demo.Console} (): Demo.Console.Write =
  console.writeLine(5, "hello")
```

If a function needs an effect and nobody hands it the capability, the door rejects it with `unhandled-effect`. There is no way around it, at run time or at build time. There are no build scripts.

## Declaring a capability

A `capability` lists its operations:

```scala
// prompt: a place to write lines of text.
capability Demo.Console:
  def writeLine(size: U64, text: Array[U8](size)): Demo.Console.Write
```

## Handling it

A `handler` implements a capability, and `handle` installs it for the rest of a function:

```scala
// prompt: a console over standard output.
handler Demo.StandardOutput() for Demo.Console:
  def writeLine(size: U64, text: Array[U8](size)): Demo.Console.Write = Demo.Posix.writeLine(1, size, text)

// prompt: the program entry.
pub def main(): I32 =
  handle console = Demo.StandardOutput()
  Demo.greet{console}() |> Demo.exitStatus
```

Tests swap in their own handlers, so the same code runs against a fake console, a fake clock or a fake network, with no mocking framework.

## Narrowing

Capabilities can be narrowed, for example `Files.under("./data")` or `Socket.to("api.example.com")`. That is how Stigi can print a proved [permission manifest](/door/authority/).

Next: [resource protocols](/language/protocols/).
