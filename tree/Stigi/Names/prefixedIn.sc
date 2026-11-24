// prompt: whether a text starts with a builtin prefix and continues with a non-empty rest that is a line of a closed name table.
def Stigi.Names.prefixedIn(tables: Stigi.Syntax.Tables, first: U64, end: U64, count: U64, prefix: Array[U8](count), at: U64, size: U64, names: Array[U8](size), budget: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 then false else
    if count <= at then first < end && Stigi.Names.tableHas(tables, first, end, size, names, 0, budget, budget) else
      if end <= first then false else
        val Tables(source, _, _, _, _, _) = tables
        if source(first) == prefix(at) then Stigi.Names.prefixedIn(tables, first + 1, end, count, prefix, at + 1, size, names, budget, remaining - 1) else false
