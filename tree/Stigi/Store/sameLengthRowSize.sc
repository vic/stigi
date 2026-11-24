// prompt: maximum byte size of one sameLength row for a unit name slice.
def Stigi.Store.sameLengthRowSize(first: U64, endPos: U64) <lessOrEqual(first, endPos)>: U64 =
  val nameLen: U64 = endPos - first
  if nameLen < 4096 then nameLen + 22 + (nameLen + 22) + 2 else 8238
