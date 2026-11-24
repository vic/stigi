// prompt: decode the byte after a backslash in a character literal when it names a single-letter escape.
def Stigi.Syntax.simpleEscape(escByte: U64): Stigi.Syntax.CharDecoded =
  if escByte == 110 then CharDecoded(10) else if escByte == 116 then CharDecoded(9) else if escByte == 92 then CharDecoded(92) else if escByte == 34 then CharDecoded(34) else if escByte == 39 then CharDecoded(39) else if escByte == 48 then CharDecoded(0) else CharMalformed
