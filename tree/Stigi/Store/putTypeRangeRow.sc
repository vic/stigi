// prompt: write one typeRange fact row into a bounded buffer at a position, giving the new end.
def Stigi.Store.putTypeRangeRow {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, primitive: Stigi.Core.Primitive, low: Stigi.Exact.IntWide, high: Stigi.Exact.IntWide) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val named: U64 = Stigi.Store.putPrimitiveName(bufCap, buf, at, primitive)
  val firstTab: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, bufCap, buf, named)
  val lowEnd: U64 = Stigi.Store.putWideNumber{region}(bufCap, buf, firstTab, low)
  val secondTab: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, bufCap, buf, lowEnd)
  val highEnd: U64 = Stigi.Store.putWideNumber{region}(bufCap, buf, secondTab, high)
  Stigi.Array.copy[U8](1, "\n", 0, 1, bufCap, buf, highEnd)
