// prompt: the index of the last dot in a byte run of the name pool, or the start of the run when it holds no dot.
def Stigi.Names.namespaceEnd(pool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64, remaining: U64) <lessOrEqual(end, Stigi.Limit.maxIndexBytes), decreases(remaining)>: U64 =
  if remaining < 1 || end <= first then first else
    if pool(end - 1) == 46 then end - 1 else Stigi.Names.namespaceEnd(pool, first, end - 1, remaining - 1)
