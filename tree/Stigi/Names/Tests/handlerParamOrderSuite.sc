// prompt: verify that the type of a value parameter of a handler sees the parameters before it and not the ones after it, as the seed binds the parameters one by one.
val Stigi.Names.Tests.handlerParamOrderSuite {Stigi.Test}: Stigi.Test.Result =
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
    val textFirst: Array[U8](52) = "// prompt: demo.\ncapability A.W:\n  def didIt(): U64\n"
    val textEarlier: Array[U8](130) = "// prompt: demo.\npub handler A.H(first: U64, second: Array[U8](first)) for A.W:\n  var cursor: U64 = 0\n  def didIt(): U64 = cursor\n"
    val textAhead: Array[U8](130) = "// prompt: demo.\npub handler A.H(second: Array[U8](first), first: U64) for A.W:\n  var cursor: U64 = 0\n  def didIt(): U64 = cursor\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](3, nameFirst, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](52, textFirst, 0, 52, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 52)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](3, nameSecond, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextEarlier: U64 = Stigi.Array.copy[U8](130, textEarlier, 0, 130, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, _) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 3, srcBuf, 130)) else
      case IndexFailed(_) => 99
    val Parsed(earlierRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 130) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    tasks(0) := Visit(earlierRoot)
    val Walker(_, _, _, _, _, _, unresolvedEarlier, _, fault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageSecond, membersSecond, tables, 1)
    val sound: Bool = fault is WalkSound
    val copiedTextAhead: U64 = Stigi.Array.copy[U8](130, textAhead, 0, 130, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(aheadRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 130) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    tasks(0) := Visit(aheadRoot)
    val Walker(_, _, _, _, _, _, unresolvedAhead, _, _) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageSecond, membersSecond, tables, 1)
    if copiedNameFirst == 3 && copiedTextFirst == 52 && copiedNameSecond == 3 && copiedTextEarlier == 130 && copiedTextAhead == 130 then
      if sound && unresolvedEarlier == 0 then
        if unresolvedAhead == 1 then 0 else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a handler value parameter type sees the earlier parameter and not the one after it") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause earlier parameter resolves") else if code == 3 then Stigi.Test.expect(false, true, "clause parameter after is unresolved") else Stigi.Test.expect(false, true, "clause setup of the handler parameter order suite")
