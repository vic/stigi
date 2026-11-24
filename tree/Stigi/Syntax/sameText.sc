// prompt: whether the output table holds the same bytes as the source table from a position up to a size.
def Stigi.Syntax.sameText(tables: Stigi.Syntax.Tables, index: U64, size: U64, remaining: U64) <lessOrEqual(size, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  val Tables(source, _, _, _, _, output) = tables
  if remaining < 1 || size <= index then true else if source(index) != output(index) then false else Stigi.Syntax.sameText(tables, index + 1, size, remaining - 1)
