// prompt: verify that the names image finds units by bare and prefixed names and misses unknown names.
val Stigi.Names.Tests.imageSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => false
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val names: Array[U8](12) = "A.B.c\nA.d\nx\n"
    val unitText: Array[U8](53) = "// prompt: demo.\ndef A.B.c(x: U64): U64 =\n  d(x) + y\n"
    val copied: U64 = Stigi.Array.copy[U8](12, names, 0, 12, Stigi.Limit.maxIndexBytes, pool, 0)
    val empty: Stigi.Names.Image = Image(pool, entries, 0, slots)
    val shape: Stigi.Names.UnitShape = Shaped(DefKind, false)
    val first: Stigi.Names.Added = Stigi.Names.addEntry(empty, 0, 5, shape)
    val checked: Bool = first match
      case Added(one) => Stigi.Names.addEntry(one, 6, 9, shape) match
        case Added(two) => Stigi.Names.addEntry(two, 10, 11, shape) match
          case Added(three) => Stigi.Syntax.Parse.parseUnit(tables, 53, unitText, 53) match
            case Parsed(_, _, _) =>
              Stigi.Names.probeName(three, tables, Prefixed(0, 3, 44, 45), Stigi.Names.candidateSlot(pool, tables, Prefixed(0, 3, 44, 45)), Stigi.Names.tableSlots) is UnitAbsent && Stigi.Names.probeName(three, tables, Prefixed(0, 1, 44, 45), Stigi.Names.candidateSlot(pool, tables, Prefixed(0, 1, 44, 45)), Stigi.Names.tableSlots) is UnitFound(1) && Stigi.Names.probeName(three, tables, Bare(51, 52), Stigi.Names.candidateSlot(pool, tables, Bare(51, 52)), Stigi.Names.tableSlots) is UnitAbsent && Stigi.Names.probeName(three, tables, Prefixed(0, 1, 51, 52), Stigi.Names.candidateSlot(pool, tables, Prefixed(0, 1, 51, 52)), Stigi.Names.tableSlots) is UnitAbsent && Stigi.Names.probeName(three, tables, Bare(27, 28), Stigi.Names.candidateSlot(pool, tables, Bare(27, 28)), Stigi.Names.tableSlots) is UnitFound(2) && Stigi.Names.entryMatches(pool, 0, 5, tables, Prefixed(0, 3, 44, 45)) == false && Stigi.Names.entryMatches(pool, 0, 5, tables, Prefixed(0, 3, 51, 52)) == false && Stigi.Names.entryMatches(pool, 0, 5, tables, Prefixed(0, 2, 24, 26)) == false
            case ParseRejected(_, _) | LexRejected(_) => false
          case ImageFull | UnitUnshaped => false
        case ImageFull | UnitUnshaped => false
      case ImageFull | UnitUnshaped => false
    copied == 12 && checked
  Stigi.Test.expect(passed, true, "the names image finds units by bare and prefixed names and misses unknown names")
