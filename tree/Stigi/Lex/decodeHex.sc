// prompt: decode an ASCII byte into a proved hexadecimal nibble below sixteen.
def Stigi.Lex.decodeHex(byte: U8): Stigi.Lex.HexDigit =
  case '0'..'9' =>
    val nibble: U64 = U64.ofU8(byte - '0')
    if nibble < 16 then HexValue(nibble@<below(nibble, 16)>) else NotHex
  case 'a'..'f' =>
    val nibble: U64 = U64.ofU8(byte - 'a') + 10
    if nibble < 16 then HexValue(nibble@<below(nibble, 16)>) else NotHex
  case 'A'..'F' =>
    val nibble: U64 = U64.ofU8(byte - 'A') + 10
    if nibble < 16 then HexValue(nibble@<below(nibble, 16)>) else NotHex
  case _ => NotHex
