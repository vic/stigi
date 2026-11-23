// prompt: test whether an ASCII byte is an uppercase letter.
def Stigi.Lex.isUpperByte(byte: U8): Bool =
  byte >= 65 && byte <= 90
