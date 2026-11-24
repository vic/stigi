// prompt: write one lengthTerm or linearForm row into a bounded buffer, returning the new end.
def Stigi.Store.putLengthAtomRow {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, unitSlice: Stigi.Store.NameSlice, termIdx: U64) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val labeled: U64 = Stigi.Store.putTermLabel{region}(bufCap, buf, at, unitSlice, termIdx)
  Stigi.Array.copy[U8](1, "\n", 0, 1, bufCap, buf, labeled)
