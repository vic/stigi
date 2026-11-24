---
title: What Stigi leaves out
description: The features we left out on purpose, and what to write instead.
sidebar:
  order: 13
---

Some familiar features are not in Stigi. Each one is left out for a reason, and each has a Stigi form. When you use one, the door replies `not-in-language` with a help line that shows the Stigi way.

| You might write | In Stigi |
|---|---|
| `import` | Use the full name of the unit. |
| `class`, `object` | An `enum`, a `type` and plain `def`s. |
| `throw`, `try` | Return a failure case as a value. |
| `null`, `Option` from a library | An enum with two cases. |
| `return`, `break`, `continue` | The last line of a block is its value; loops end on their guard. |
| `for` comprehensions | A `while` loop. |
| `Vec.push` | `Array.tryGrow`. |
| A recursive type | A table of nodes with `U64` indexes. |
| String interpolation | Write the pieces one by one. |
| Varargs | An array with its length. |
| A C ABI other than `"c"` | `extern "c"`. |

None of these are missing by accident. Each one either hides control flow, hides an effect, or makes facts harder to prove. Leaving them out keeps every program small enough for the door to check, and plain enough for a human to read.

That is the language. Next, let's see what happens when code meets [the door](/door/image/).
