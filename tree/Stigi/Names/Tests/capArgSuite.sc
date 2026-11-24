// prompt: verify that a capability argument of a call is a reference by name that binds nothing, so an operation of the handle binder after the call still adds the edges to the handler, the capability and the callee with nothing unresolved, a capability unit named as an argument adds its edge, and an operation the capability does not have still fails the walk.
val Stigi.Names.Tests.capArgSuite {Stigi.Test}: Stigi.Test.Result =
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
    val Filled(sourcesTwo) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(targetsTwo) = Array.tryFill[U64]{Stigi.Region}(Stigi.Names.maxEdges, 0) else
      case FillFailed => 99
    val Filled(stampsTwo) = Array.tryFill[U64]{Stigi.Region}(Stigi.Limit.maxUnits, 0) else
      case FillFailed => 99
    val nameFirst: Array[U8](3) = "A.W"
    val nameSecond: Array[U8](3) = "A.H"
    val nameThird: Array[U8](3) = "A.g"
    val nameFourth: Array[U8](3) = "A.k"
    val nameFifth: Array[U8](3) = "A.f"
    val textFirst: Array[U8](52) = "// prompt: demo.\ncapability A.W:\n  def didIt(): U64\n"
    val textSecond: Array[U8](107) = "// prompt: demo.\npub handler A.H(capLimit: U64) for A.W:\n  var cursor: U64 = 0\n  def didIt(): U64 = cursor\n"
    val textThird: Array[U8](45) = "// prompt: demo.\ndef A.g {A.W} (): U64 =\n  0\n"
    val textFourth: Array[U8](45) = "// prompt: demo.\ndef A.k {A.W} (): U64 =\n  0\n"
    val textFifth: Array[U8](80) = "// prompt: demo.\ndef A.f(): U64 =\n  handle w = A.H(8192)\n  A.g{w}()\n  w.didIt()\n"
    val textMissing: Array[U8](82) = "// prompt: demo.\ndef A.f(): U64 =\n  handle w = A.H(8192)\n  A.g{w}()\n  w.nothing()\n"
    val textNamed: Array[U8](47) = "// prompt: demo.\ndef A.f(): U64 =\n  A.k{A.W}()\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](3, nameFirst, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](52, textFirst, 0, 52, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 52)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](3, nameSecond, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextSecond: U64 = Stigi.Array.copy[U8](107, textSecond, 0, 107, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, endSecond) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 3, srcBuf, 107)) else
      case IndexFailed(_) => 99
    val copiedNameThird: U64 = Stigi.Array.copy[U8](3, nameThird, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextThird: U64 = Stigi.Array.copy[U8](45, textThird, 0, 45, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageThird, membersThird, endThird) = Stigi.Names.indexUnit(imageSecond, membersSecond, tables, endSecond, UnitSource(nameBuf, 3, srcBuf, 45)) else
      case IndexFailed(_) => 99
    val copiedNameFourth: U64 = Stigi.Array.copy[U8](3, nameFourth, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFourth: U64 = Stigi.Array.copy[U8](45, textFourth, 0, 45, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFourth, membersFourth, endFourth) = Stigi.Names.indexUnit(imageThird, membersThird, tables, endThird, UnitSource(nameBuf, 3, srcBuf, 45)) else
      case IndexFailed(_) => 99
    val copiedNameFifth: U64 = Stigi.Array.copy[U8](3, nameFifth, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFifth: U64 = Stigi.Array.copy[U8](80, textFifth, 0, 80, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFifth, membersFifth, _) = Stigi.Names.indexUnit(imageFourth, membersFourth, tables, endFourth, UnitSource(nameBuf, 3, srcBuf, 80)) else
      case IndexFailed(_) => 99
    val Parsed(root, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 80) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    tasks(0) := Visit(root)
    val Walker(_, _, _, _, walked, _, unresolved, _, fault) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 4)
    val sound: Bool = fault is WalkSound
    val copiedTextMissing: U64 = Stigi.Array.copy[U8](82, textMissing, 0, 82, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(missingRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 82) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val failed: Bool = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 4, missingRoot) match
      case Walked(_) => false
      case WalkFailed(why) => why is UnresolvedNames
    val copiedTextNamed: U64 = Stigi.Array.copy[U8](47, textNamed, 0, 47, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Parsed(namedRoot, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 47) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    tasks(0) := Visit(namedRoot)
    val Walker(_, _, _, _, walkedNamed, _, unresolvedNamed, _, _) = Stigi.Names.walkFrom(Walker(tasks, 1, locals, 0, Edges(sourcesTwo, targetsTwo, 0, stampsTwo), 0, 0, 0, WalkSound), imageFifth, membersFifth, tables, 4)
    if copiedNameFirst == 3 then
      if copiedTextFirst == 52 then
        if copiedNameSecond == 3 then
          if copiedTextSecond == 107 then
            if copiedNameThird == 3 then
              if copiedTextThird == 45 then
                if copiedNameFourth == 3 then
                  if copiedTextFourth == 45 then
                    if copiedNameFifth == 3 then
                      if copiedTextFifth == 80 then
                        if copiedTextMissing == 82 then
                          if copiedTextNamed == 47 then
                            if sound then
                              if unresolved == 0 then
                                if Stigi.Names.Tests.hasEdge(walked, 4, 1) then
                                  if Stigi.Names.Tests.hasEdge(walked, 4, 0) then
                                    if Stigi.Names.Tests.hasEdge(walked, 4, 2) then
                                      if failed then
                                        if unresolvedNamed == 0 then
                                          if Stigi.Names.Tests.hasEdge(walkedNamed, 4, 0) then
                                            if Stigi.Names.Tests.hasEdge(walkedNamed, 4, 3) then 0 else 21
                                          else 20
                                        else 19
                                      else 18
                                    else 17
                                  else 16
                                else 15
                              else 14
                            else 13
                          else 12
                        else 11
                      else 10
                    else 9
                  else 8
                else 7
              else 6
            else 5
          else 4
        else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a capability argument of a call is a reference that binds nothing: the binder operation after the call adds edges to the handler, the capability and the callee, a capability unit argument adds its edge and an unknown operation fails the walk") else if code == 1 then Stigi.Test.expect(false, true, "clause first name copied") else if code == 2 then Stigi.Test.expect(false, true, "clause first text copied") else if code == 3 then Stigi.Test.expect(false, true, "clause second name copied") else if code == 4 then Stigi.Test.expect(false, true, "clause second text copied") else if code == 5 then Stigi.Test.expect(false, true, "clause third name copied") else if code == 6 then Stigi.Test.expect(false, true, "clause third text copied") else if code == 7 then Stigi.Test.expect(false, true, "clause fourth name copied") else if code == 8 then Stigi.Test.expect(false, true, "clause fourth text copied") else if code == 9 then Stigi.Test.expect(false, true, "clause fifth name copied") else if code == 10 then Stigi.Test.expect(false, true, "clause fifth text copied") else if code == 11 then Stigi.Test.expect(false, true, "clause missing text copied") else if code == 12 then Stigi.Test.expect(false, true, "clause named text copied") else if code == 13 then Stigi.Test.expect(false, true, "clause walk is sound after the call") else if code == 14 then Stigi.Test.expect(false, true, "clause nothing unresolved after the call") else if code == 15 then Stigi.Test.expect(false, true, "clause edge to the handler") else if code == 16 then Stigi.Test.expect(false, true, "clause edge to the capability") else if code == 17 then Stigi.Test.expect(false, true, "clause edge to the callee") else if code == 18 then Stigi.Test.expect(false, true, "clause unknown operation fails with unresolved names") else if code == 19 then Stigi.Test.expect(false, true, "clause nothing unresolved for the capability unit argument") else if code == 20 then Stigi.Test.expect(false, true, "clause capability unit argument adds its capability edge") else if code == 21 then Stigi.Test.expect(false, true, "clause capability unit argument adds its callee edge") else Stigi.Test.expect(false, true, "clause setup of the capability argument suite")
