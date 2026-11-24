// prompt: the hash of a bare name kept in the member pool, equal to the hash of that name stored in the name pool.
def Stigi.Names.poolBareHash(memberPool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: U64 =
  Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, memberPool, first, end, false, Stigi.Names.hashSeed, end - first + 1)
