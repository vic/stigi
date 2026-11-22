// prompt: format an unsigned 64-bit integer in base ten into the output buffer.
pub def Stigi.Emit.writeUnsignedDecimal {region: Stigi.Region, Stigi.Mut} (value: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val zeroByte: U8 = 0
  Array.tryFill{region}(24, zeroByte) match
    case FillFailed => writeOffset
    case Filled(digits) =>
      var digitCount: U64 = 0
      var remaining: U64 = value
      var done: Bool = false
      while digitCount < 24 && !done do
        @<lessOrEqual(digitCount, 24), decreases(24 - digitCount)>
        val lowDigit: U64 = remaining % 10
        val asciiByte: U8 = U8.ofU64(U64.bitwiseAnd(lowDigit + 48, 255))
        val revSlot: U64 = 23 - digitCount
        digits(revSlot) := asciiByte
        remaining = remaining / 10
        digitCount = digitCount + 1
        if remaining == 0 then
          done = true
        else
          ()
      var readIndex: U64 = 24 - digitCount
      var pos: U64 = writeOffset
      while readIndex < 24 do
        @<lessOrEqual(readIndex, 24), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(24 - readIndex)>
        pos = Stigi.Emit.writeByte(outCap, output, pos, digits(readIndex))
        readIndex = readIndex + 1
      pos
