// prompt: compare a byte run of the name pool with a text of the current unit.
def Stigi.Names.poolHasText(pool: Array[U8](Stigi.Limit.maxIndexBytes), at: U64, tables: Stigi.Syntax.Tables, first: U64, end: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 then false else
    if end <= first then true else
      if Stigi.Limit.maxIndexBytes <= at then false else
        val Tables(source, _, _, _, _, _) = tables
        if pool(at) == source(first) then Stigi.Names.poolHasText(pool, at + 1, tables, first + 1, end, remaining - 1) else false
