// prompt: write one sameLength row into a bounded buffer, returning the new end.
def Stigi.Store.putSameLengthRow {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, unitSlice: Stigi.Store.NameSlice, pair: Stigi.Store.LengthPair) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val LengthPair(leftIdx, rightIdx) = pair
  val firstEnd: U64 = Stigi.Store.putTermLabel{region}(bufCap, buf, at, unitSlice, leftIdx)
  val tabbed: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, bufCap, buf, firstEnd)
  val secondEnd: U64 = Stigi.Store.putTermLabel{region}(bufCap, buf, tabbed, unitSlice, rightIdx)
  Stigi.Array.copy[U8](1, "\n", 0, 1, bufCap, buf, secondEnd)
