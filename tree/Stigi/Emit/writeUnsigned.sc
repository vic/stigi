// prompt: write an unsigned sixty-four-bit integer in decimal ASCII via Stigi.Emit.Writer.
def Stigi.Emit.writeUnsigned {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), value: U64): Unit =
  if value == 0 then
    writer.putByte(outCap, outBuf, 48)
  else
    val Filled(digits) = Array.tryFill[U8]{region}(24, 0) else
      case FillFailed => ()
    var cur: U64 = value
    var count: U64 = 0
    while cur > 0 && count < 24 do
      @<lessOrEqual(count, 24)>
      val digit: U64 = cur % 10
      val asciiByte: U64 = 48 + digit
      if asciiByte < 256 then digits(count) := U8.ofU64(asciiByte) else ()
      cur = cur / 10
      count = count + 1
    var revIdx: U64 = count
    while revIdx > 0 do
      @<lessOrEqual(revIdx, count)>
      revIdx = revIdx - 1
      writer.putByte(outCap, outBuf, digits(revIdx))
