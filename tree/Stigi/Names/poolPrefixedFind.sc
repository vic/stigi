// prompt: find the unit of an image that carries a name kept in the member pool behind a namespace prefix of the name pool.
def Stigi.Names.poolPrefixedFind(image: Stigi.Names.Image, memberPool: Array[U8](Stigi.Limit.maxIndexBytes), prefixFirst: U64, prefixEnd: U64, first: U64, end: U64) <lessOrEqual(prefixFirst, prefixEnd), lessOrEqual(prefixEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Stigi.Names.UnitLookup =
  image match
    case Image(pool, _, _, _) => Stigi.Names.poolPrefixedProbe(image, memberPool, PoolRun(prefixFirst, prefixEnd), first, end, Stigi.Names.poolPrefixedHash(pool, memberPool, prefixFirst, prefixEnd, first, end) % Stigi.Names.tableSlots, Stigi.Names.tableSlots)
