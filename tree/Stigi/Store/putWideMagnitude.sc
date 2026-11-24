// prompt: write an unsigned 128-bit magnitude in decimal digits into a bounded buffer at a position, giving the new end.
def Stigi.Store.putWideMagnitude {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, magnitude: Stigi.Exact.UnsignedWide) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val UnsignedPair(magLow, magHigh) = magnitude
  if magHigh == 0 then Stigi.Store.putUnsigned{region}(bufCap, buf, at, magLow) else
    val Filled(digits) = Array.tryFill[U8]{region}(40, 0) else
      case FillFailed => at
    var curLow: U64 = magLow
    var curHigh: U64 = magHigh
    var pos: U64 = 40
    while 0 < pos && (0 < curHigh || 0 < curLow || pos == 40) do
      @<lessOrEqual(pos, 40)>
      pos = pos - 1
      val UnsignedDivPair(quotLow, quotHigh, remLow, _) = Stigi.Exact.divideUnsigned(UnsignedPair(curLow, curHigh), 10, 128)
      val asciiByte: U64 = 48 + remLow % 10
      if asciiByte < 256 then digits(pos) := U8.ofU64(asciiByte) else ()
      curLow = quotLow
      curHigh = quotHigh
    Stigi.Array.copy[U8](40, digits, pos, 40, bufCap, buf, at)
