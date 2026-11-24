---
title: A two-minute tour
description: A few lines of Stigi, and everything the door proves about them.
sidebar:
  order: 2
---

Stigi borrows the clean, indentation-based look of Scala 3. If you read Python, Kotlin, Scala or Rust, you can read Stigi.

## Counting bytes

```scala
// prompt: count how many bytes in a block match a target byte.
pub def Bytes.countMatches(size: U64, block: Array[U8](size), target: U8): U64 <lessOrEqual(result, size)> =
  var matches: U64 = 0
  var index: U64 = 0
  while index < size do
    @<lessOrEqual(matches, index), lessOrEqual(index, size)>
    matches = if block(index) == target then matches + 1 else matches
    index = index + 1
  matches
```

Three things to notice:

- The first line is a `// prompt:`. Every unit says, in plain words, what it is for.
- The length of `block` is part of its type: `Array[U8](size)`.
- The facts in `<...>` are checked at compile time, not at run time.

Now look at what is **not** there. There is no bounds check on `block(index)`, because the loop guard already proves `index < size`. There is no overflow check on `matches + 1`, because `matches` stays below `size`. Callers get `lessOrEqual(result, size)` as a proved fact they can use right away.

## When something is wrong

Say an agent reads `table(index)` without proving that `index` is in range. Nothing runs, and nothing gets into the image. The door answers with the missing fact and a fix it already checked:

```text
rejected {Demo.Table.peek}: index-unproved at Demo.Table.peek.body[0]
  need: below(index, count)
  fix1 (checked): add entry fact <below(index, count)>
  lesson: index-unproved (see cached prefix)
```

The agent replies `accept fix1`, and the unit is in.

## Errors and resources

```scala
// prompt: read a bounded header from an open file into a fresh buffer, then close the file.
pub def Storage.readHeader {region: Stigi.Region, file: Stigi.File} <at(file, Open)> (limit: U64)
    <lessOrEqual(limit, 4096)>: Storage.HeaderResult <at(file, Closed)> =
  val Filled(buffer) = Array.tryFill{region}(limit, 0) else case FillFailed =>
    val _ = file.close()
    OutOfMemory
  val outcome: Stigi.File.Read = file.read(limit, buffer)
  val _ = file.close()
  outcome match
    case BytesRead(count) => HeaderLoaded(count, buffer)
    case ReadFailed(code) => StorageError(code)
```

Running out of memory is just a value, `FillFailed`, never a crash. You never write `free`, and you never write lifetimes. If you forget `file.close()` on any path, the door refuses the unit, because the file must end `Closed`.

That is the whole idea. The rest of these docs take each piece one step at a time, starting with [units and prompts](/language/units/).
