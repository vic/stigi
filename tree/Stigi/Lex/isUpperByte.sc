// prompt: check whether one byte is an uppercase ASCII letter.
def Stigi.Lex.isUpperByte(byte: U8) <lessOrEqual(0, 1)>: Bool =
  byte >= 65 && byte <= 90
