// prompt: maximum byte size of one lengthTerm or linearForm row for a unit name slice.
def Stigi.Store.lengthAtomRowSize(first: U64, endPos: U64) <lessOrEqual(first, endPos)>: U64 =
  val nameLen: U64 = endPos - first
  if nameLen < 4096 then nameLen + 23 else 4119
