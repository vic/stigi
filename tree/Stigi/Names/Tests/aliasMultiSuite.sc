// prompt: verify that a reference equal to an alias name, a reference that starts with an alias name before a dot, and a reference with several dots after an alias name each rewrite to the unit the target names.
val Stigi.Names.Tests.aliasMultiSuite {Stigi.Test}: Stigi.Test.Result =
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
    val names: Array[U8](20) = "A.B.c\nC.x\nC.x.y\nA.d\n"
    val copied: U64 = Stigi.Array.copy[U8](20, names, 0, 20, Stigi.Limit.maxIndexBytes, pool, 0)
    val Added(first) = Stigi.Names.addEntry(Image(pool, entries, 0, slots), 0, 5, Shaped(DefKind, false)) else
      case ImageFull | UnitUnshaped => 99
    val Added(second) = Stigi.Names.addEntry(first, 6, 9, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val Added(third) = Stigi.Names.addEntry(second, 10, 15, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val Added(fourth) = Stigi.Names.addEntry(third, 16, 19, Shaped(DefKind, true)) else
      case ImageFull | UnitUnshaped => 99
    val sample: Array[U8](86) = "// prompt: demo.\nalias Q = A.d\nalias P = C\ndef A.B.c(x: U64): U64 =\n  Q + P.x + P.x.y\n"
    val Parsed(root, _, _) = Stigi.Syntax.Parse.parseUnit(tables, 86, sample, 86) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val members: Stigi.Names.Members = Members(memberPool, 0, table, 0, memberSlots)
    tasks(0) := Visit(root)
    val Walker(_, _, _, _, walked, _, unresolved, _, fault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), fourth, members, tables, 0)
    val Edges(edgeSources, edgeTargets, count, _) = walked
    val sound: Bool = fault is WalkSound
    if copied == 20 then
      if sound then
        if count == 3 then
          if unresolved == 0 then
            if edgeSources(0) == 0 && edgeTargets(0) == 3 then
              if edgeSources(1) == 0 && edgeTargets(1) == 1 then
                if edgeSources(2) == 0 && edgeTargets(2) == 2 then 0 else 8
              else 7
            else 6
          else 5
        else 4
      else 3
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "an exact alias, a prefix alias and a multi dot alias reference each add an edge to the unit they name") else if code == 1 then Stigi.Test.expect(false, true, "clause copied names") else if code == 3 then Stigi.Test.expect(false, true, "clause the walk is sound") else if code == 4 then Stigi.Test.expect(false, true, "clause edge count is three") else if code == 5 then Stigi.Test.expect(false, true, "clause unresolved count is zero") else if code == 6 then Stigi.Test.expect(false, true, "clause exact alias edge") else if code == 7 then Stigi.Test.expect(false, true, "clause prefix alias edge") else if code == 8 then Stigi.Test.expect(false, true, "clause multi dot alias edge") else Stigi.Test.expect(false, true, "clause setup of the alias multi suite")
