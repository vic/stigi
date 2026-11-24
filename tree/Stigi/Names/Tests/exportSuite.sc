// prompt: verify that five units indexed one by one and walked one by one give five entries and the exact edges, including the edges a relation and its rule owe each other, that a walk fault fails the verdict and that a unit referencing a name that exists nowhere fails with unresolved names.
val Stigi.Names.Tests.exportSuite {Stigi.Test}: Stigi.Test.Result =
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
    val nameFirst: Array[U8](5) = "A.B.c"
    val nameSecond: Array[U8](3) = "A.d"
    val nameThird: Array[U8](3) = "C.x"
    val nameFourth: Array[U8](5) = "A.rel"
    val nameFifth: Array[U8](3) = "A.r"
    val textFirst: Array[U8](51) = "// prompt: demo.\ndef A.B.c(x: U64): U64 =\n  d(C.x)\n"
    val textSecond: Array[U8](44) = "// prompt: demo.\ndef A.d(x: U64): U64 =\n  x\n"
    val textThird: Array[U8](38) = "// prompt: demo.\npub val C.x: U64 = 1\n"
    val textFourth: Array[U8](40) = "// prompt: demo.\nrelation A.rel(x: U64)\n"
    val textFifth: Array[U8](51) = "// prompt: demo.\nrule def A.r: A.rel(x) = A.rel(x)\n"
    val textSixth: Array[U8](53) = "// prompt: demo.\ndef A.e(x: U64): U64 =\n  nowhere(x)\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](5, nameFirst, 0, 5, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](51, textFirst, 0, 51, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 5, srcBuf, 51)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](3, nameSecond, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextSecond: U64 = Stigi.Array.copy[U8](44, textSecond, 0, 44, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, endSecond) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 3, srcBuf, 44)) else
      case IndexFailed(_) => 99
    val copiedNameThird: U64 = Stigi.Array.copy[U8](3, nameThird, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextThird: U64 = Stigi.Array.copy[U8](38, textThird, 0, 38, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageThird, membersThird, endThird) = Stigi.Names.indexUnit(imageSecond, membersSecond, tables, endSecond, UnitSource(nameBuf, 3, srcBuf, 38)) else
      case IndexFailed(_) => 99
    val copiedNameFourth: U64 = Stigi.Array.copy[U8](5, nameFourth, 0, 5, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFourth: U64 = Stigi.Array.copy[U8](40, textFourth, 0, 40, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFourth, membersFourth, endFourth) = Stigi.Names.indexUnit(imageThird, membersThird, tables, endThird, UnitSource(nameBuf, 5, srcBuf, 40)) else
      case IndexFailed(_) => 99
    val copiedNameFifth: U64 = Stigi.Array.copy[U8](3, nameFifth, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFifth: U64 = Stigi.Array.copy[U8](51, textFifth, 0, 51, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFifth, membersFifth, endFifth) = Stigi.Names.indexUnit(imageFourth, membersFourth, tables, endFourth, UnitSource(nameBuf, 3, srcBuf, 51)) else
      case IndexFailed(_) => 99
    val againFirst: U64 = Stigi.Array.copy[U8](51, textFirst, 0, 51, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootFirst, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 51) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesFirst) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 0, rootFirst) else
      case WalkFailed(_) => 99
    val againSecond: U64 = Stigi.Array.copy[U8](44, textSecond, 0, 44, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootSecond, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 44) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesSecond) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edgesFirst, 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 1, rootSecond) else
      case WalkFailed(_) => 99
    val againThird: U64 = Stigi.Array.copy[U8](38, textThird, 0, 38, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootThird, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 38) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesThird) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edgesSecond, 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 2, rootThird) else
      case WalkFailed(_) => 99
    val againFourth: U64 = Stigi.Array.copy[U8](40, textFourth, 0, 40, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootFourth, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 40) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesFourth) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edgesThird, 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 3, rootFourth) else
      case WalkFailed(_) => 99
    val againFifth: U64 = Stigi.Array.copy[U8](51, textFifth, 0, 51, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootFifth, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 51) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val Walked(edgesFifth) = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edgesFourth, 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 4, rootFifth) else
      case WalkFailed(_) => 99
    val againSixth: U64 = Stigi.Array.copy[U8](53, textSixth, 0, 53, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(rootSixth, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 53) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val unresolvedFails: Bool = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, edgesFifth, 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 1, rootSixth) match
      case WalkFailed(UnresolvedNames) => true
      case WalkFailed(_) | Walked(_) => false
    val Edges(_, _, count, _) = edgesFifth
    val Image(_, _, entryCount, _) = imageFifth
    val faulted: Bool = Stigi.Names.walkVerdict(TableMalformed, 0, edgesFifth) match
      case Walked(_) => false
      case WalkFailed(_) => true
    val sound: Bool = Stigi.Names.walkVerdict(WalkSound, 0, edgesFifth) match
      case Walked(_) => true
      case WalkFailed(_) => false
    if copiedNameFirst == 5 && copiedTextFirst == 51 && copiedNameSecond == 3 && copiedTextSecond == 44 && copiedNameThird == 3 && copiedTextThird == 38 && copiedNameFourth == 5 && copiedTextFourth == 40 && copiedNameFifth == 3 && copiedTextFifth == 51 then
      if againFirst == 51 && againSecond == 44 && againThird == 38 && againFourth == 40 && againFifth == 51 && againSixth == 53 && endFifth == 19 then
        if entryCount == 5 then
          if count == 4 then
            if Stigi.Names.Tests.hasEdge(edgesFifth, 0, 1) && Stigi.Names.Tests.hasEdge(edgesFifth, 0, 2) then
              if Stigi.Names.Tests.hasEdge(edgesFifth, 3, 4) && Stigi.Names.Tests.hasEdge(edgesFifth, 4, 3) then
                if faulted && sound && unresolvedFails then 0 else 7
              else 6
            else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "five units indexed and walked give five entries and the def edges, the rule edge from its relation and the reference back, a fault fails the verdict and an unresolved name fails the walk") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause reparsed bytes and pool end") else if code == 3 then Stigi.Test.expect(false, true, "clause entry count is five") else if code == 4 then Stigi.Test.expect(false, true, "clause edge count is four") else if code == 5 then Stigi.Test.expect(false, true, "clause def edges") else if code == 6 then Stigi.Test.expect(false, true, "clause rule edges") else if code == 7 then Stigi.Test.expect(false, true, "clause walk verdicts and unresolved names") else Stigi.Test.expect(false, true, "clause setup of the export suite")
