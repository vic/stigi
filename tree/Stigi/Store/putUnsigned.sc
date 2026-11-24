// prompt: write an unsigned 64-bit integer in decimal digits into a bounded buffer at a position, giving the new end.
def Stigi.Store.putUnsigned {region: Stigi.Region, Stigi.Mut} (bufCap: Stigi.EmitCap, buf: Array[U8](bufCap), at: U64, value: U64) <lessOrEqual(at, bufCap)>: U64 <lessOrEqual(result, bufCap)> =
  val Filled(digits) = Array.tryFill[U8]{region}(20, 0) else
    case FillFailed => at
  var rest: U64 = value
  var pos: U64 = 20
  while 0 < pos && (0 < rest || pos == 20) do
    @<lessOrEqual(pos, 20)>
    pos = pos - 1
    val asciiByte: U64 = 48 + rest % 10
    if asciiByte < 256 then digits(pos) := U8.ofU64(asciiByte) else ()
    rest = rest / 10
  Stigi.Array.copy[U8](20, digits, pos, 20, bufCap, buf, at)
