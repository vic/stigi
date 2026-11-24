// prompt: verify that an alias name followed by more letters, with or without a dot, is not a reference to the alias target even when the glued name exists, and fails the walk with unresolved names.
val Stigi.Names.Tests.aliasBoundarySuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => 99
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => 99
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => 99
    val Filled(memberPool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => 99
    val Filled(table) = Array.tryFill[Stigi.Names.Member]{Stigi.Region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
      case FillFailed => 99
    val Filled(memberSlots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => 99
    val Filled(tasks) = Array.tryFill[Stigi.Names.Task]{Stigi.Region}(Stigi.Names.maxScope, Restore(0)) else
      case FillFailed => 99
    val Filled(locals) = Array.tryFill[Stigi.Names.Local]{Stigi.Region}(Stigi.Names.maxScope, Local(Text(0, 0), WithoutCapability)) else
      case FillFailed => 99
    val Filled(sources) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(targets) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(stamps) = Array.tryFill[U64]{Stigi.Region}(Stigi.Limit.maxUnits, 0) else
      case FillFailed => 99
    val names: Array[U8](18) = "A.B.c\nC.x\nCQ.x\nCx\n"
    val copied: U64 = Stigi.Array.copy[U8](18, names, 0, 18, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(first) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(second) = Stigi.Names.addEntry(first, 6, 9, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val Added(third) = Stigi.Names.addEntry(second, 10, 14, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val Added(fourth) = Stigi.Names.addEntry(third, 15, 17, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val members: Stigi.Names.Members = Members(memberPool, 0, table, 0, memberSlots)
    val dotted: Array[U8](61) = "// prompt: demo.\nalias P = C\ndef A.B.c(x: U64): U64 =\n  PQ.x\n"
    val Parsed(rootDotted, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 61, dotted, 61) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val dottedFails: Bool = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), fourth, members, tables, 0, rootDotted) match
      case WalkFailed(UnresolvedNames) => true
      case WalkFailed(_) | Walked(_) => false
    val bare: Array[U8](59) = "// prompt: demo.\nalias P = C\ndef A.B.c(x: U64): U64 =\n  Px\n"
    val Parsed(rootBare, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 59, bare, 59) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val bareFails: Bool = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), fourth, members, tables, 0, rootBare) match
      case WalkFailed(UnresolvedNames) => true
      case WalkFailed(_) | Walked(_) => false
    if copied == 18 then
      if dottedFails then
        if bareFails then 0 else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a reference that only starts with an alias name is not rewritten even when the glued name exists and fails with unresolved names") else if code == 1 then Stigi.Test.expect(false, true, "clause copied names") else if code == 2 then Stigi.Test.expect(false, true, "clause alias name before more letters and a dot") else if code == 3 then Stigi.Test.expect(false, true, "clause alias name before more letters") else Stigi.Test.expect(false, true, "clause setup of the alias boundary suite")
