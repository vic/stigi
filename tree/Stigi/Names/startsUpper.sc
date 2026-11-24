// prompt: whether a source run starts with an uppercase ASCII letter, the mark of a constructor name.
def Stigi.Names.startsUpper(tables: Stigi.Syntax.Tables, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Bool =
  val Tables(source, _, _, _, _, _) = tables
  if first < end then source(first) is 'A'..'Z' else false
