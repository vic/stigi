// prompt: whether the source bytes of a text equal the builtin name line that starts at an offset of the builtin text.
def Stigi.Names.builtinLineIs(tables: Stigi.Syntax.Tables, first: U64, end: U64, names: Array[U8](Stigi.Names.builtinTextBytes), at: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 || Stigi.Names.builtinTextBytes <= at then false else
    if names(at) == 10 then first == end else
      if end <= first then false else
        val Tables(source, _, _, _, _, _) = tables
        if source(first) == names(at) then Stigi.Names.builtinLineIs(tables, first + 1, end, names, at + 1, remaining - 1) else false
