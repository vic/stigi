// prompt: the index of the last dot in a text of the current unit, or the start of the text when it holds no dot.
def Stigi.Names.lastDot(source: Array[U8](Stigi.Limit.maxUnitBytes), first: U64, end: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: U64 =
  if remaining < 1 || end <= first then first else
    if source(end - 1) == '.' then end - 1 else Stigi.Names.lastDot(source, first, end - 1, remaining - 1)
