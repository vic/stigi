// prompt: round a byte request up to 64-byte cache-line alignment for 64 bytes or larger, otherwise 8-byte alignment.
pub def Stigi.Arena.alignBlock(bytes: U64) <below(bytes, 0x100000000)>: U64 <lessOrEqual(bytes, result)> =
  if bytes >= 64 then
    Stigi.Types.alignForward(bytes, 64)
  else
    Stigi.Types.alignForward(bytes, 8)
