// prompt: convert a signed 64-bit integer to its two-complement unsigned bit pattern.
pub def Stigi.Exact.signedToBits(value: I64): U64 =
  if value < 0 then
    val shifted: I64 = value + 9223372036854775807
    if shifted < 0 then
      9223372036854775808
    else
      @<lessOrEqual(0, shifted)>
      val baseBits: U64 = U64.ofI64(shifted)
      U64.wrappingAdd(baseBits, 9223372036854775809)
  else
    U64.ofI64(value)
