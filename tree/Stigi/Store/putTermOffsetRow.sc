// prompt: write one termOffset row into a bounded buffer, returning the new end.
def Stigi.Store.putTermOffsetRow {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, unitSlice: Stigi.Store.NameSlice, pair: Stigi.Store.LengthPair, offset: U64) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val LengthPair(termIdx, baseIdx) = pair
  val firstEnd: U64 = Stigi.Store.putTermLabel{region}(bufCap, buf, at, unitSlice, termIdx)
  val tabOne: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, bufCap, buf, firstEnd)
  val secondEnd: U64 = Stigi.Store.putTermLabel{region}(bufCap, buf, tabOne, unitSlice, baseIdx)
  val tabTwo: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, bufCap, buf, secondEnd)
  val offEnd: U64 = Stigi.Store.putUnsigned{region}(bufCap, buf, tabTwo, offset)
  Stigi.Array.copy[U8](1, "\n", 0, 1, bufCap, buf, offEnd)
