// prompt: verify that a bare capability group and a named capability binder both walk soundly and each owe an edge to the capability unit.
val Stigi.Names.Tests.capabilitySuite {Stigi.Test}: Stigi.Test.Result =
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
    val Filled(nameBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => 99
    val Filled(srcBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => 99
    val nameFirst: Array[U8](5) = "A.Mut"
    val nameSecond: Array[U8](3) = "A.f"
    val nameThird: Array[U8](3) = "A.g"
    val textFirst: Array[U8](35) = "// prompt: demo.\ncapability A.Mut:\n"
    val textSecond: Array[U8](53) = "// prompt: demo.\ndef A.f {A.Mut} (x: U64): U64 =\n  x\n"
    val textThird: Array[U8](56) = "// prompt: demo.\ndef A.g {m: A.Mut} (x: U64): U64 =\n  x\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](5, nameFirst, 0, 5, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](35, textFirst, 0, 35, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 5, srcBuf, 35)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](3, nameSecond, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextSecond: U64 = Stigi.Array.copy[U8](53, textSecond, 0, 53, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, endSecond) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 3, srcBuf, 53)) else
      case IndexFailed(_) => 99
    val copiedNameThird: U64 = Stigi.Array.copy[U8](3, nameThird, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextThird: U64 = Stigi.Array.copy[U8](56, textThird, 0, 56, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageThird, membersThird, _) = Stigi.Names.indexUnit(imageSecond, membersSecond, tables, endSecond, UnitSource(nameBuf, 3, srcBuf, 56)) else
      case IndexFailed(_) => 99
    val againSecond: U64 = Stigi.Array.copy[U8](53, textSecond, 0, 53, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootSecond, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 53) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesSecond) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageThird, membersThird, tables, 1, rootSecond) else
      case WalkFailed(_) => 99
    val againThird: U64 = Stigi.Array.copy[U8](56, textThird, 0, 56, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootThird, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 56) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesThird) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edgesSecond, 0, 0, 0, WalkSound), imageThird, membersThird, tables, 2, rootThird) else
      case WalkFailed(_) => 99
    val Edges(_, _, count, _) = edgesThird
    if copiedNameFirst == 5 && copiedTextFirst == 35 && copiedNameSecond == 3 && copiedTextSecond == 53 && copiedNameThird == 3 && copiedTextThird == 56 && againSecond == 53 && againThird == 56 then
      if count == 2 then
        if Stigi.Names.Tests.hasEdge(edgesThird, 1, 0) && Stigi.Names.Tests.hasEdge(edgesThird, 2, 0) then 0 else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a bare capability group and a named capability binder walk soundly and each give one edge to the capability unit") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause edge count is two") else if code == 3 then Stigi.Test.expect(false, true, "clause edges to the capability") else Stigi.Test.expect(false, true, "clause setup of the capability suite")
