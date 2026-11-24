// prompt: verify the named limit that the binder of a handle statement whose handler is for a builtin capability carries no operations, so an operation called on it stays unresolved and fails the walk with the typed reason.
val Stigi.Names.Tests.builtinHandlerSuite {Stigi.Test}: Stigi.Test.Result =
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
    val nameFirst: Array[U8](3) = "A.H"
    val nameSecond: Array[U8](3) = "A.f"
    val textFirst: Array[U8](115) = "// prompt: demo.\npub handler A.H(capLimit: U64) for Stigi.Clock:\n  var cursor: U64 = 0\n  def didIt(): U64 = cursor\n"
    val textSecond: Array[U8](69) = "// prompt: demo.\ndef A.f(): U64 =\n  handle w = A.H(8192)\n  w.didIt()\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](3, nameFirst, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](115, textFirst, 0, 115, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 115)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](3, nameSecond, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextSecond: U64 = Stigi.Array.copy[U8](69, textSecond, 0, 69, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, _) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 3, srcBuf, 69)) else
      case IndexFailed(_) => 99
    val Parsed(root, _, _) = Stigi.Syntax.Parse.parseUnit(tables, Stigi.Limit.maxUnitBytes, srcBuf, 69) else
      case ParseRejected(_, _) | LexRejected(_) => 99
    val failed: Bool = Stigi.Names.walkUnit(Walker(tasks, 0, locals, 0, Edges(sources, targets, 0, stamps), 0, 0, 0, WalkSound), imageSecond, membersSecond, tables, 1, root) match
      case Walked(_) => false
      case WalkFailed(why) => why is UnresolvedNames
    if copiedNameFirst == 3 && copiedTextFirst == 115 && copiedNameSecond == 3 && copiedTextSecond == 69 then
      if failed then 0 else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "an operation called on the binder of a handler for a builtin capability fails the walk with unresolved names") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause operation on a builtin capability binder fails with unresolved names") else Stigi.Test.expect(false, true, "clause setup of the builtin handler suite")
