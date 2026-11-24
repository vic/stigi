---
title: Units and prompts
description: The smallest piece of a Stigi program.
sidebar:
  order: 1
---

A Stigi program is a set of named **units**. A unit is one definition: a function, a value, an enum, a type and so on.

```scala
// prompt: the larger of two numbers.
def Demo.larger(left: U64, right: U64): U64 =
  if left < right then right else left
```

## The prompt line

Every unit starts with a `// prompt:` line that says, in plain words, what the unit must do. It is not optional. It is the first thing a human or an agent reads, and it stays with the unit forever.

## Names are full words

A dotted name like `Demo.larger` is one name. The part before the last dot is its namespace. Units never import each other: a unit uses another by its full name.

Names are made of real words. `descriptor`, not `fd`. `index`, not `ix`. Full English words are easier for people to read, and they are cheaper for language models too, because a common word is usually a single token.

## Indentation, not braces

A block is the indented lines after a line that ends in `=`, `then`, `else`, `do`, `=>` or `:`. Two spaces per level. The value of a block is its last line. There is no `;` and no `return`.

## Public or private

Units are private to their namespace by default. Add `pub` only when another namespace needs it:

```scala
// prompt: the larger of two numbers, for other namespaces.
pub def Demo.max(left: U64, right: U64): U64 = Demo.larger(left, right)
```

Next: [numbers, arrays and types](/language/types/).
