// prompt: whether one image entry carries a name kept in the member pool behind a namespace prefix of the name pool.
def Stigi.Names.poolPrefixedMatches(pool: Array[U8](Stigi.Limit.maxIndexBytes), entry: Stigi.Names.Entry, memberPool: Array[U8](Stigi.Limit.maxIndexBytes), prefix: Stigi.Names.PoolRun, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Bool =
  val Entry(entryFirst, entryEnd, _, _) = entry
  val PoolRun(prefixFirst, prefixEnd) = prefix
  val prefixLength: U64 = prefixEnd - prefixFirst
  val dotAt: U64 = entryFirst + prefixLength
  if entryEnd - entryFirst == prefixLength + 1 + (end - first) && Stigi.Names.poolSame(pool, entryFirst, prefixFirst, prefixLength) then
    if dotAt < entryEnd then pool(dotAt) == '.' && Stigi.Names.crossSame(pool, dotAt + 1, entryEnd, memberPool, first, end) else false
  else false
