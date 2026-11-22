// prompt: decode an unsigned zigzag word back into a signed 64-bit integer with proved range safety.
pub def Stigi.Identity.zigzagDecode(encoded: U64) <lessOrEqual(0, 1)>: I64 =
  val half: U64 = U64.shiftRight(encoded, 1)
  val mag: I64 = I64.ofU64(half)
  if U64.bitwiseAnd(encoded, 1) == 0 then
    mag
  else
    if mag >= 0 then 0 - mag - 1 else mag
