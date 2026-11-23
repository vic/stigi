// prompt: decode an ASCII byte into a proved hexadecimal nibble below sixteen.
def Stigi.Lex.decodeHex(byte: U8): Stigi.Lex.HexDigit =
  val word: U64 = U64.ofU8(byte)
  if word >= 48 && word <= 57 then
    val nibble: U64 = word - 48
    if nibble < 16 then HexValue(nibble@<below(nibble, 16)>) else NotHex
  else if word >= 97 && word <= 102 then
    val nibble: U64 = word - 87
    if nibble < 16 then HexValue(nibble@<below(nibble, 16)>) else NotHex
  else if word >= 65 && word <= 70 then
    val nibble: U64 = word - 55
    if nibble < 16 then HexValue(nibble@<below(nibble, 16)>) else NotHex
  else NotHex
