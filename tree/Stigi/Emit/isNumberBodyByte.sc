// prompt: check whether an ASCII byte continues a decimal or hexadecimal integer literal.
def Stigi.Emit.isNumberBodyByte(byteVal: U8) <lessOrEqual(0, 1)>: Bool =
  Stigi.Lex.isDigitByte(byteVal) || byteVal >= 97 && byteVal <= 102 || byteVal >= 65 && byteVal <= 70 || byteVal == 120 || byteVal == 88
