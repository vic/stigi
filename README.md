# Stigi

[![Docs](https://img.shields.io/badge/docs-stigi.org-cba6f7)](https://stigi.org)
[![Sponsor](https://img.shields.io/badge/sponsor-vic-ea4aaa?logo=githubsponsors)](https://github.com/sponsors/vic)

**A systems language for the era when AI writes most of the code, and humans still have to trust it, run it and understand it.**

Stigi reads like clean Scala 3 and compiles to plain, portable C11. There is no garbage collector, no runtime panic and no out-of-memory kill. Every piece of code enters a program through one door, and before anything gets in, one Datalog engine proves that memory is used safely, that indexes stay in bounds, that numbers never overflow, that the stack never runs out and that files and sockets are always closed.

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

No bounds check and no overflow check in the emitted C: both are proved. The [two-minute tour](https://stigi.org/start/tour/) walks through what the door knows about this little function.

Resources follow rules too. A file can only be read while it is open, and it must be closed on every path:

```scala
// prompt: read the first bytes of an open file, then close it.
pub def Storage.readFirst {file: Stigi.File} <at(file, Open)> (size: U64, buffer: Array[U8](size))
    : Stigi.File.Read <at(file, Closed)> =
  val outcome = file.read(size, buffer)
  val _ = file.close()
  outcome
```

Swap the two middle lines and nothing runs. The door answers:

```text
rejected {Storage.readFirst}: op-not-allowed at Storage.readFirst.body[1]
  need:  at(file, Open)
  found: at(file, Closed)
```

Forget `close()` on any path, and you get `capability-not-ended` instead. Read about [resource protocols](https://stigi.org/language/protocols/).

> [!NOTE]
> **Stigi is a work in progress.** It is an ambitious project, and I build it in the open, on my own time and with my own money. If you want to see it happen sooner, [sponsoring it](https://github.com/sponsors/vic) is the most direct way to help. Leaving a star, telling a friend, or trying it on a real problem and [telling us what breaks](https://github.com/vic/stigi/issues) helps a lot too. Thank you :)

## Read more

Everything lives at **[stigi.org](https://stigi.org)**, in small pages that go from easy to advanced:

- [Why Stigi exists](https://stigi.org/start/why/): the problem, in five minutes.
- [The language](https://stigi.org/language/units/): units, facts, loops, errors, memory, capabilities and laws.
- [The door](https://stigi.org/door/image/): one reasoning engine, no slop, checked complexity, and the [law loop](https://stigi.org/door/law-loop/) that stops agents from cheating.
- [Working with agents](https://stigi.org/agents/session/): a small, cheap protocol built around tokens.
- [Understanding code](https://stigi.org/humans/theory-sheet/): theory sheets and theory diffs instead of walls of text.
- [The runtime envelope](https://stigi.org/runtime/envelope/): proved resources, safe overcommit, replay and fork.
- [Trust](https://stigi.org/trust/self-hosting/): how the checker is checked, and [what Stigi does not claim](https://stigi.org/trust/limits/).
- [Related work](https://stigi.org/related/): every project and paper we learned from.

## In this repository

- [`src/`](./src) is the seed compiler, written in C, used to bootstrap.
- [`tree/`](./tree) is Stigi's own source, the self-hosted compiler, one unit per file.
- [`docs/`](./docs) is the website.

```console
$ git clone https://github.com/vic/stigi
$ cd stigi
$ nix develop
```

## License

Stigi is free software under the [GNU AGPL](./LICENSE), so the tools that verify AI-written software stay open for everyone. Read [why](https://stigi.org/community/sponsor/), and how to sponsor or become an early adopter.
