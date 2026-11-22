// prompt: convert a four-bit nibble to its lowercase hexadecimal ASCII byte.
def Stigi.Emit.hexNibbleByte(nibble: U64) <below(nibble, 16)>: U8 =
  if nibble < 10 then
    U8.ofU64(U64.bitwiseAnd(nibble + 48, 255))
  else
    U8.ofU64(U64.bitwiseAnd(nibble + 87, 255))
