// prompt: write an exact signed 128-bit integer in decimal into a bounded buffer at a position, giving the new end.
def Stigi.Store.putWideNumber {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, value: Stigi.Exact.IntWide) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val WideInt(_, highWord) = value
  val afterSign: U64 = if highWord < 0 then Stigi.Array.copy[U8](1, "-", 0, 1, bufCap, buf, at) else at
  Stigi.Store.putWideMagnitude{region}(bufCap, buf, afterSign, Stigi.Exact.magnitudeOf(value))
