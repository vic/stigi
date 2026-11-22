// prompt: check whether one byte continues a decimal or hexadecimal integer literal.
def Stigi.Lex.isNumberByte(byte: U8) <lessOrEqual(0, 1)>: Bool =
  Stigi.Lex.isDigitByte(byte) || byte == 120 || byte == 88 || byte >= 97 && byte <= 102 || byte >= 65 && byte <= 70
