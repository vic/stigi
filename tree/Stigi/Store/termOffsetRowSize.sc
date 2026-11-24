// prompt: maximum byte size of one termOffset row for a unit name slice.
def Stigi.Store.termOffsetRowSize(first: U64, endPos: U64) <lessOrEqual(first, endPos)>: U64 =
  val nameLen: U64 = endPos - first
  if nameLen < 4096 then nameLen + 22 + (nameLen + 22) + 23 else 8259
