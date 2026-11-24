// prompt: whether the source bytes of a text equal one of the lines of a closed name table, scanning from an offset.
def Stigi.Names.tableHas(tables: Stigi.Syntax.Tables, first: U64, end: U64, size: U64, names: Array[U8](size), at: U64, budget: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 || size <= at then false else
    if Stigi.Names.tableLineIs(tables, first, end, size, names, at, budget) then true else Stigi.Names.tableHas(tables, first, end, size, names, Stigi.Names.nextTableLine(size, names, at, budget), budget, remaining - 1)
