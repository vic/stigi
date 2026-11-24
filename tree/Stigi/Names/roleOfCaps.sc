// prompt: the role the capability group of a val gives it, scanning the group from one list entry on.
def Stigi.Names.roleOfCaps(tables: Stigi.Syntax.Tables, first: U64, end: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitNodes), decreases(remaining)>: Stigi.Names.ValRole =
  if remaining < 1 || end <= first then PlainValue else
    val Tables(_, _, _, lists, _, _) = tables
    val capAt: U64 = lists(first)
    val role: Stigi.Names.ValRole = if capAt < Stigi.Limit.maxUnitNodes then Stigi.Names.roleOfCap(tables, capAt) else PlainValue
    role match
      case PlainValue => Stigi.Names.roleOfCaps(tables, first + 1, end, remaining - 1)
      case TestValue => TestValue
      case NegTestValue => NegTestValue
