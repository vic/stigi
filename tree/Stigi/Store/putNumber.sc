// prompt: write an unsigned number in decimal digits into the report text at a position, giving the new end.
def Stigi.Store.putNumber {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), at: U64, value: U64) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
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
  Stigi.Array.copy[U8](20, digits, pos, 20, 65536, report, at)
