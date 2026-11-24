// prompt: verify that a handler records the capability name it stands for, that the name found in the member pool resolves from the namespace of the handler to the capability unit, that an unknown name stays missing and that a capability has no such name.
val Stigi.Names.Tests.handlerPoolSuite {Stigi.Test}: Stigi.Test.Result =
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
    val nameFirst: Array[U8](3) = "A.W"
    val nameSecond: Array[U8](5) = "A.B.H"
    val nameThird: Array[U8](5) = "A.B.G"
    val textFirst: Array[U8](33) = "// prompt: demo.\ncapability A.W:\n"
    val textSecond: Array[U8](40) = "// prompt: demo.\nhandler A.B.H() for W:\n"
    val textThird: Array[U8](43) = "// prompt: demo.\nhandler A.B.G() for Nope:\n"
    val copiedNameFirst: U64 = Stigi.Array.copy[U8](3, nameFirst, 0, 3, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextFirst: U64 = Stigi.Array.copy[U8](33, textFirst, 0, 33, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageFirst, membersFirst, endFirst) = Stigi.Names.indexUnit(Image(pool, entries, 0, slots), Members(memberPool, 0, table, 0, memberSlots), tables, 0, UnitSource(nameBuf, 3, srcBuf, 33)) else
      case IndexFailed(_) => 99
    val copiedNameSecond: U64 = Stigi.Array.copy[U8](5, nameSecond, 0, 5, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextSecond: U64 = Stigi.Array.copy[U8](40, textSecond, 0, 40, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageSecond, membersSecond, endSecond) = Stigi.Names.indexUnit(imageFirst, membersFirst, tables, endFirst, UnitSource(nameBuf, 5, srcBuf, 40)) else
      case IndexFailed(_) => 99
    val copiedNameThird: U64 = Stigi.Array.copy[U8](5, nameThird, 0, 5, Stigi.Limit.maxUnitBytes, nameBuf, 0)
    val copiedTextThird: U64 = Stigi.Array.copy[U8](43, textThird, 0, 43, Stigi.Limit.maxUnitBytes, srcBuf, 0)
    val Indexed(imageThird, membersThird, _) = Stigi.Names.indexUnit(imageSecond, membersSecond, tables, endSecond, UnitSource(nameBuf, 5, srcBuf, 43)) else
      case IndexFailed(_) => 99
    val resolved: Bool = Stigi.Names.forCapability(membersThird, 1, 0, Stigi.Names.maxMembers) match
      case MissingFor => false
      case ForText(first, end) =>
        Stigi.Names.searchPoolName(imageThird, membersThird, 1, first, end) match
          case UnitFound(slot) => slot == 0
          case UnitAbsent => false
    val missed: Bool = Stigi.Names.forCapability(membersThird, 2, 0, Stigi.Names.maxMembers) match
      case MissingFor => false
      case ForText(first, end) =>
        Stigi.Names.searchPoolName(imageThird, membersThird, 2, first, end) match
          case UnitFound(_) => false
          case UnitAbsent => true
    val capabilityHasNone: Bool = Stigi.Names.forCapability(membersThird, 0, 0, Stigi.Names.maxMembers) match
      case MissingFor => true
      case ForText(_, _) => false
    if copiedNameFirst == 3 && copiedTextFirst == 33 && copiedNameSecond == 5 && copiedTextSecond == 40 && copiedNameThird == 5 && copiedTextThird == 43 then
      if resolved then
        if missed && capabilityHasNone then 0 else 3
      else 2
    else 1
  if code == 0 then Stigi.Test.expect(true, true, "a handler records its capability name, the name resolves from the handler namespace to the capability unit, an unknown name stays missing and a capability has no such name") else if code == 1 then Stigi.Test.expect(false, true, "clause copied bytes") else if code == 2 then Stigi.Test.expect(false, true, "clause the handler capability name resolves to the capability unit") else if code == 3 then Stigi.Test.expect(false, true, "clause unknown name missing and capability without name") else Stigi.Test.expect(false, true, "clause setup of the handler pool suite")
