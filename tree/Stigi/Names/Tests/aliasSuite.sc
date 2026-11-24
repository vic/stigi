// prompt: verify that an alias rewritten name is found by the namepath search of its two runs: the alias target and the tail of the reference.
val Stigi.Names.Tests.aliasSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => false
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val names: Array[U8](14) = "A.B.c\nA.d\nC.x\n"
    val copied: U64 = Stigi.Array.copy[U8](14, names, 0, 14, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(alpha) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(beta) = Stigi.Names.addEntry(alpha, 6, 9, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => false
    val Added(gamma) = Stigi.Names.addEntry(beta, 10, 13, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => false
    val unitText: Array[U8](82) = "// prompt: demo.\nalias P = C\nalias Q = A.d\ndef A.B.c(x: U64): U64 =\n  P.x + Q + P\n"
    val Parsed(_, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 82, unitText, 82) else
      case ParseRejected(_, _) | LexRejected(_) => false
    val found: Bool = Stigi.Names.searchAliased(gamma, tables, 0, 27, 28, 71, 73) is Found(2) && Stigi.Names.searchAliased(gamma, tables, 0, 39, 42, 77, 77) is Found(1) && Stigi.Names.searchAliased(gamma, tables, 1, 39, 42, 77, 77) is Found(1)
    val missed: Bool = Stigi.Names.searchAliased(gamma, tables, 0, 27, 28, 77, 77) is Missing && Stigi.Names.searchAliased(gamma, tables, 0, 27, 28, 71, 72) is Missing
    val direct: Bool = Stigi.Names.entryMatches(pool, Entry(10, 13, DefKind, false), tables, Joined(27, 28, 71, 73)) && Stigi.Names.entryMatches(pool, Entry(10, 13, DefKind, false), tables, Joined(27, 28, 73, 75)) == false && Stigi.Names.entryMatches(pool, Entry(6, 9, DefKind, false), tables, PrefixedJoined(0, 1, 41, 42, 77, 77))
    copied == 14 && found && missed && direct
  Stigi.Test.expect(passed, true, "an alias rewritten name is found by the namepath search of the alias target joined with the tail of the reference")
