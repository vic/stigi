// prompt: encode a signed 64-bit integer into unsigned zigzag form with proved overflow safety.
pub def Stigi.Identity.zigzagEncode(signedWord: I64) <lessOrEqual(0, 1)>: U64 =
  if signedWord < 0 then
    val shifted: U64 = U64.shiftLeft(U64.ofI64(0 - (signedWord + 1)), 1)
    U64.bitwiseOr(shifted, 1)
  else
    U64.shiftLeft(U64.ofI64(signedWord), 1)
