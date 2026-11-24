// prompt: whether one image entry carries a bare name kept in the member pool.
def Stigi.Names.poolBareMatches(pool: Array[U8](Stigi.Limit.maxIndexBytes), entry: Stigi.Names.Entry, memberPool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Bool =
  val Entry(entryFirst, entryEnd, _, _) = entry
  Stigi.Names.crossSame(pool, entryFirst, entryEnd, memberPool, first, end)
