// prompt: check whether one byte continues a dotted identifier.
def Stigi.Lex.isIdentByte(byte: U8) <lessOrEqual(0, 1)>: Bool =
  Stigi.Lex.isLetterByte(byte) || Stigi.Lex.isDigitByte(byte) || byte == 46 || byte == 95
