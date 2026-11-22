// prompt: encode one unsigned 64-bit integer in minimal LEB128 into an output buffer.
pub def Stigi.Identity.encodeUnsignedLeb {Stigi.Mut} (value: U64, outCap: U64, output: Array[U8](outCap), offset: U64) <lessOrEqual(offset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(offset, result), lessOrEqual(result, outCap)> =
  var remaining: U64 = value
  var outCursor: U64 = offset
  var steps: U64 = 0
  var finished: Bool = false
  while steps < 10 && outCursor < outCap && !finished do
    @<lessOrEqual(offset, outCursor), lessOrEqual(outCursor, outCap), lessOrEqual(steps, 10), decreases(10 - steps)>
    val lowSeven: U64 = U64.bitwiseAnd(remaining, 127)
    val nextRem: U64 = U64.shiftRight(remaining, 7)
    if nextRem == 0 then
      output(outCursor) := U8.ofU64(U64.bitwiseAnd(lowSeven, 255))
      outCursor = outCursor + 1
      finished = true
    else
      val contByte: U64 = U64.bitwiseAnd(U64.bitwiseOr(lowSeven, 128), 255)
      output(outCursor) := U8.ofU64(contByte)
      outCursor = outCursor + 1
      remaining = nextRem
    steps = steps + 1
  outCursor
