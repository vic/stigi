// prompt: fold one ASCII uppercase letter byte to lowercase using native bit operations.
pub def Stigi.TenetGate.toLowerByte(byte: U8) <lessOrEqual(0, 1)>: U8 =
  if byte >= 65 && byte <= 90 then
    val lowered: U64 = U64.bitwiseAnd(U64.bitwiseOr(U64.ofU8(byte), 32), 255)
    U8.ofU64(lowered)
  else
    byte
