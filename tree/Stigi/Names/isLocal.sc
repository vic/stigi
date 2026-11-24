// prompt: whether a source run names a local binder of the walk of a unit.
def Stigi.Names.isLocal(tables: Stigi.Syntax.Tables, walker: Stigi.Names.Walker, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Bool =
  val Walker(_, _, locals, localCount, _, _, _, _, _) = walker
  Stigi.Names.localAt(tables, locals, localCount, first, end, localCount)
