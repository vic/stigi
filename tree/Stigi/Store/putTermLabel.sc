// prompt: write one canonical term identifier unitName:l<termIdx> into a bounded buffer, returning the new end.
def Stigi.Store.putTermLabel {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, unitSlice: Stigi.Store.NameSlice, termIdx: U64) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val NameSlice(pool, first, endPos) = unitSlice
  val named: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxIndexBytes, pool, first, endPos, bufCap, buf, at)
  val marked: U64 = Stigi.Array.copy[U8](2, ":l", 0, 2, bufCap, buf, named)
  Stigi.Store.putUnsigned{region}(bufCap, buf, marked, termIdx)
