// prompt: find the unit of an image that carries a bare name kept in the member pool.
def Stigi.Names.poolBareFind(image: Stigi.Names.Image, memberPool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Stigi.Names.UnitLookup =
  Stigi.Names.poolBareProbe(image, memberPool, first, end, Stigi.Names.poolBareHash(memberPool, first, end) % Stigi.Names.tableSlots, Stigi.Names.tableSlots)
