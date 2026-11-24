// prompt: verify that a capability binder whose declared type sits on a slot that does not hold its own index, and a capability entry past the unit count, are malformed reads instead of no capability.
val Stigi.Names.Tests.capabilityMalformedSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(entries) = Array.tryFill[Stigi.Names.Entry]{Stigi.Region}(Stigi.Limit.maxUnits, Entry(0, 0, DefKind, false)) else
      case FillFailed => false
    val Filled(slots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val Filled(memberPool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(table) = Array.tryFill[Stigi.Names.Member]{Stigi.Region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
      case FillFailed => false
    val Filled(memberSlots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val image: Stigi.Names.Image = Image(pool, entries, 0, slots)
    val wrongSlot: Bool = Stigi.Names.capabilityOf(image, Members(memberPool, 0, table, 0, memberSlots), tables, 0, 5) is CapabilityMalformed
    val pastCount: Bool = Stigi.Names.capabilityUnit(image, 0) is CapabilityMalformed
    wrongSlot && pastCount
  Stigi.Test.expect(passed, true, "a capability binder whose declared type is on a wrong slot, and an entry past the unit count, are malformed reads")
