// prompt: verify that an alias rewritten name is found by the namepath search of its two runs, the alias target and the tail of the reference, and that the alias scan of the locals gives the target and the tail one assertion at a time.
val Stigi.Names.Tests.aliasSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => 99
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => 99
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => 99
    val Filled(locals) = Array.tryFill[Stigi.Names.Local]{Stigi.Region}(Stigi.Names.maxScope, Local(Text(0, 0), WithoutCapability)) else
      case FillFailed => 99
    val names: Array[U8](14) = "A.B.c\nA.d\nC.x\n"
    val copied: U64 = Stigi.Array.copy[U8](14, names, 0, 14, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(alpha) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(beta) = Stigi.Names.addEntry(alpha, 6, 9, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(gamma) = Stigi.Names.addEntry(beta, 10, 13, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val unitText: Array[U8](82) = "// prompt: demo.\nalias P = C\nalias Q = A.d\ndef A.B.c(x: U64): U64 =\n  P.x + Q + P\n"
    val Parsed(_, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 82, unitText, 82) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    locals(0) := AliasLocal(Text(23, 24), 27, 28)
    locals(1) := AliasLocal(Text(35, 36), 39, 42)
    if copied != 14 then 1 else if Stigi.Names.searchAliased(gamma, tables, 0, Text(27, 28), Text(71, 73)) is UnitFound(2) == false then 2 else if Stigi.Names.searchAliased(gamma, tables, 0, Text(39, 42), Text(77, 77)) is UnitFound(1) == false then 3 else if Stigi.Names.searchAliased(gamma, tables, 1, Text(39, 42), Text(77, 77)) is UnitFound(1) == false then 4 else if Stigi.Names.searchAliased(gamma, tables, 0, Text(27, 28), Text(77, 77)) is UnitAbsent == false then 5 else if Stigi.Names.searchAliased(gamma, tables, 0, Text(27, 28), Text(71, 72)) is UnitAbsent == false then 6 else if Stigi.Names.entryMatches(pool, 10, 13, tables, Joined(27, 28, 71, 73)) == false then 7 else if Stigi.Names.entryMatches(pool, 10, 13, tables, Joined(27, 28, 73, 75)) then 8 else if Stigi.Names.entryMatches(pool, 6, 9, tables, PrefixedJoined(0, 1, 41, 42, 77, 77)) == false then 9 else if Stigi.Names.aliasFor(tables, locals, 2, 70, 73, 0, 2) is AliasHit(Text(27, 28), Text(71, 73)) == false then 10 else if Stigi.Names.aliasFor(tables, locals, 2, 76, 77, 0, 2) is AliasHit(Text(39, 42), Text(77, 77)) == false then 11 else if Stigi.Names.aliasFor(tables, locals, 2, 72, 73, 0, 2) is Unaliased == false then 12 else 0
  if code == 0 then Stigi.Test.expect(true, true, "an alias rewritten name is found by the namepath search of the alias target joined with the tail of the reference") else if code == 1 then Stigi.Test.expect(false, true, "clause copied names") else if code == 2 then Stigi.Test.expect(false, true, "clause alias target and tail found") else if code == 3 then Stigi.Test.expect(false, true, "clause bare alias reference found") else if code == 4 then Stigi.Test.expect(false, true, "clause second entry found") else if code == 5 then Stigi.Test.expect(false, true, "clause wrong tail missed") else if code == 6 then Stigi.Test.expect(false, true, "clause short tail missed") else if code == 7 then Stigi.Test.expect(false, true, "clause joined entry matches") else if code == 8 then Stigi.Test.expect(false, true, "clause shifted tail does not match") else if code == 9 then Stigi.Test.expect(false, true, "clause prefixed joined entry matches") else if code == 10 then Stigi.Test.expect(false, true, "clause aliasFor gives target and tail of a dotted reference") else if code == 11 then Stigi.Test.expect(false, true, "clause aliasFor gives target of a bare reference") else if code == 12 then Stigi.Test.expect(false, true, "clause aliasFor gives unaliased for a name no alias covers") else Stigi.Test.expect(false, true, "clause setup of the alias suite")
