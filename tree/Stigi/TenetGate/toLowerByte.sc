// prompt: fold an ASCII uppercase byte to lowercase preserving non-decreasing byte order.
def Stigi.TenetGate.toLowerByte(byte: U8): U8 <lessOrEqual(byte, result)> =
  if byte >= 65 && byte <= 90 then
    val lowered: U64 = U64.ofU8(byte) + 32
    if lowered < 256 then U8.ofU64(lowered) else byte
  else byte
