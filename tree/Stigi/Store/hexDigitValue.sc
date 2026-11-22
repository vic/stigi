// prompt: decode one lowercase hexadecimal ASCII byte into a nibble value or 255 on invalid input.
def Stigi.Store.hexDigitValue(byteVal: U8) <lessOrEqual(0, 1)>: U64 =
  if byteVal >= 48 && byteVal <= 57 then
    U64.ofU8(byteVal - 48)
  else if byteVal >= 97 && byteVal <= 102 then
    U64.ofU8(byteVal - 87)
  else
    255
