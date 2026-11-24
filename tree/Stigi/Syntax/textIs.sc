// prompt: whether the source bytes of a text run equal a literal from one offset on.
pub def Stigi.Syntax.textIs(tables: Stigi.Syntax.Tables, first: U64, end: U64, size: U64, literal: Array[U8](size), offset: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 then false else
    if size <= offset then first == end else
      if end <= first then false else
        val Tables(source, _, _, _, _, _) = tables
        if source(first) == literal(offset) then Stigi.Syntax.textIs(tables, first + 1, end, size, literal, offset + 1, remaining - 1) else false
