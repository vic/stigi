// prompt: whether the source bytes of a text equal a name line that starts at an offset of a closed name table.
def Stigi.Names.tableLineIs(tables: Stigi.Syntax.Tables, first: U64, end: U64, size: U64, names: Array[U8](size), at: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 || size <= at then false else
    if names(at) == 10 then first == end else
      if end <= first then false else
        val Tables(source, _, _, _, _, _) = tables
        if source(first) == names(at) then Stigi.Names.tableLineIs(tables, first + 1, end, size, names, at + 1, remaining - 1) else false
