// prompt: verify that the binder of a handle statement with an explicit body is visible inside that body and gone after it, so a use of the binder after the body stays unresolved and fails the walk.
val Stigi.Names.Tests.handleBodySuite {Stigi.Test}: Stigi.Test.Result =
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
    val Filled(nameBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
      case FillFailed => 99
    val Filled(srcBuf) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxUnitBytes, 0) else
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
    val nameFirst: Array[U8](3) = "A.W"
    val nameSecond: Array[U8](3) = "A.H"
    val nameThird: Array[U8](3) = "A.f"
    val textFirst: Array[U8](52) = "// prompt: demo.\ncapability A.W:\n  def didIt(): U64\n"
    val textSecond: Array[U8](107) = "// prompt: demo.\npub handler A.H(capLimit: U64) for A.W:\n  var cursor: U64 = 0\n  def didIt(): U64 = cursor\n"
    val textInside: Array[U8](76) = "// prompt: demo.\ndef A.f(): U64 =\n  handle w = A.H(8192):\n    w.didIt()\n  0\n"
    val textAfter: Array[U8](84) = "// prompt: demo.\ndef A.f(): U64 =\n  handle w = A.H(8192):\n    w.didIt()\n  w.didIt()\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](3, nameFirst, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](52, textFirst, 0, 52, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 52)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](3, nameSecond, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextSecond: U64 = Stigi.Array.copy[U8](107, textSecond, 0, 107, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, endSecond) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 3, srcBuf, 107)) else
      case IndexFailed(_) => 99
    val copiedNameThird: U64 = Stigi.Array.copy[U8](3, nameThird, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextInside: U64 = Stigi.Array.copy[U8](76, textInside, 0, 76, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageThird, membersThird, _) = Stigi.Names.indexUnit(imageSecond, membersSecond, tables, endSecond, UnitSource(nameBuf, 3, srcBuf, 76)) else
      case IndexFailed(_) => 99
    val Parsed(insideRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 76) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    tasks(0) := Visit(insideRoot)
    val Walker(_, _, _, _, walked, _, unresolved, _, fault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageThird, membersThird, tables, 2)
    val sound: Bool = fault is WalkSound
    val copiedTextAfter: U64 = Stigi.Array.copy[U8](84, textAfter, 0, 84, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(afterRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 84) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val failed: Bool = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageThird, membersThird, tables, 2, afterRoot) match
      case Walked(_) => false
      case WalkFailed(why) => why is UnresolvedNames
    if copiedNameFirst == 3 && copiedTextFirst == 52 && copiedNameSecond == 3 && copiedTextSecond == 107 && copiedNameThird == 3 && copiedTextInside == 76 && copiedTextAfter == 84 then
      if sound && unresolved == 0 then
        if Stigi.Names.Tests.hasEdge(walked, 2, 1) then
          if failed then 0 else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "the binder of a handle statement with a body is visible inside the body and gone after it") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause binder resolves inside the body") else if code == 3 then Stigi.Test.expect(false, true, "clause edge to the handler") else if code == 4 then Stigi.Test.expect(false, true, "clause binder is gone after the body") else Stigi.Test.expect(false, true, "clause setup of the handle body suite")
