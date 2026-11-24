// prompt: verify that indexing a unit under the name it declares succeeds and indexing it under another name fails with a name mismatch.
val Stigi.Names.Tests.mismatchSuite {Stigi.Test}: Stigi.Test.Result =
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
    val rightName: Array[U8](3) = "A.d"
    val wrongName: Array[U8](3) = "A.q"
    val text: Array[U8](44) = "// prompt: demo.\ndef A.d(x: U64): U64 =\n  x\n"
    val copiedText: U64 = Stigi.Array.copy[U8](44, text, 0, 44, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val copiedWrong: U64 = Stigi.Array.copy[U8](3, wrongName, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val mismatched: Bool = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 44)) match
      case IndexFailed(NameMismatch) => true
      case IndexFailed(_) | Indexed(_, _, _) => false
    val copiedRight: U64 = Stigi.Array.copy[U8](3, rightName, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val matched: Bool = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 44)) match
      case Indexed(_, _, 3) => true
      case Indexed(_, _, _) | IndexFailed(_) => false
    if copiedText == 44 && copiedWrong == 3 && copiedRight == 3 then
      if mismatched then
        if matched then 0 else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a unit indexes under its declared name and fails with a name mismatch under another name") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause other name is a mismatch") else if code == 3 then Stigi.Test.expect(false, true, "clause declared name indexes") else Stigi.Test.expect(false, true, "clause setup of the mismatch suite")
