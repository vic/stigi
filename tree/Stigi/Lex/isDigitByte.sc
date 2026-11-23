// prompt: test whether an ASCII byte is a decimal digit.
def Stigi.Lex.isDigitByte(byte: U8): Bool =
  byte >= 48 && byte <= 57
