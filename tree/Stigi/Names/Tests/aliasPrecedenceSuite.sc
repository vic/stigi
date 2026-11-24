// prompt: verify first match wins over exactness: of the aliases P = C then P.x = D.x, the first one matches the reference P.x as a prefix before a dot (names.c alias scan: aliases in declaration order, a match is the whole name or the name before a dot, the first match returns), so the edge goes to C.x and none to D.x.
val Stigi.Names.Tests.aliasPrecedenceSuite {Stigi.Test}: Stigi.Test.Result =
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
    val names: Array[U8](14) = "A.B.c\nC.x\nD.x\n"
    val copied: U64 = Stigi.Array.copy[U8](14, names, 0, 14, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(first) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(second) = Stigi.Names.addEntry(first, 6, 9, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val Added(third) = Stigi.Names.addEntry(second, 10, 13, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val sample: Array[U8](76) = "// prompt: demo.\nalias P = C\nalias P.x = D.x\ndef A.B.c(x: U64): U64 =\n  P.x\n"
    val Parsed(root, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 76, sample, 76) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val members: Stigi.Names.Members = Members(memberPool, 0, table, 0, memberSlots)
    tasks(0) := Visit(root)
    val Walker(_, _, _, _, walked, _, unresolved, _, fault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), third, members, tables, 0)
    val Edges(edgeSources, edgeTargets, count, _) = walked
    val sound: Bool = fault is WalkSound
    if copied == 14 then
      if sound then
        if count == 1 then
          if unresolved == 0 then
            if edgeSources(0) == 0 && edgeTargets(0) == 1 then 0 else 6
          else 5
        else 4
      else 3
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "the first prefix alias wins over the following exact alias") else if code == 1 then Stigi.Test.expect(false, true, "clause copied names") else if code == 3 then Stigi.Test.expect(false, true, "clause the walk is sound") else if code == 4 then Stigi.Test.expect(false, true, "clause edge count is one") else if code == 5 then Stigi.Test.expect(false, true, "clause unresolved count is zero") else if code == 6 then Stigi.Test.expect(false, true, "clause the edge names the first prefix target") else Stigi.Test.expect(false, true, "clause setup of the alias precedence suite")
