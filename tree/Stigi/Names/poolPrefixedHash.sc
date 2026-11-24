// prompt: the hash of a name kept in the member pool behind a namespace prefix of the name pool, equal to the hash of the full name stored in the name pool.
def Stigi.Names.poolPrefixedHash(pool: Array[U8](Stigi.Limit.maxIndexBytes), memberPool: Array[U8](Stigi.Limit.maxIndexBytes), prefixFirst: U64, prefixEnd: U64, first: U64, end: U64) <lessOrEqual(prefixFirst, prefixEnd), lessOrEqual(prefixEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: U64 =
  val prefixHash: U64 = Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, prefixFirst, prefixEnd, false, Stigi.Names.hashSeed, prefixEnd - prefixFirst + 1)
  val dotHash: U64 = Stigi.Names.hashStep(prefixHash, U64.ofU8('.'))
  Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, memberPool, first, end, false, dotHash, end - first + 1)
