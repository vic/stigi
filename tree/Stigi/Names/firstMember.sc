// prompt: find the first member of the member table whose text equals a text of the current unit.
def Stigi.Names.firstMember(members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.MemberStep =
  val Tables(source, _, _, _, _, _) = tables
  Stigi.Names.nextMember(members, tables, first, end, Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, first, end, false, Stigi.Names.hashSeed, end - first + 1) % Stigi.Names.tableSlots, Stigi.Names.tableSlots)
