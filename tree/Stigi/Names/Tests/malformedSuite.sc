// prompt: verify that recording a span whose list entry names a slot that does not hold its own index, or a slot past the node table, reports a malformed table instead of success.
val Stigi.Names.Tests.malformedSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Allocated(tables) = Stigi.Syntax.allocate{Stigi.Region}() else
      case AllocationFailed => false
    val Filled(memberPool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
      case FillFailed => false
    val Filled(table) = Array.tryFill[Stigi.Names.Member]{Stigi.Region}(Stigi.Names.maxMembers, Member(0, CaseMember, 0, 0)) else
      case FillFailed => false
    val Filled(memberSlots) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(Stigi.Names.tableSlots, Vacant) else
      case FillFailed => false
    val Tables(_, _, _, lists, _, _) = tables
    lists(0) := 5
    lists(1) := Stigi.Limit.maxUnitNodes
    val members: Stigi.Names.Members = Members(memberPool, 0, table, 0, memberSlots)
    val wrongSlot: Bool = Stigi.Names.recordSpan(members, tables, 0, Span(0, 1)) is MembersMalformed
    val pastTable: Bool = Stigi.Names.recordSpan(members, tables, 0, Span(1, 2)) is MembersMalformed
    wrongSlot && pastTable
  Stigi.Test.expect(passed, true, "a span entry on a wrong slot or past the node table is a malformed table")
