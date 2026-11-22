// prompt: check whether one byte is a lowercase ASCII letter.
def Stigi.Lex.isLowerByte(byte: U8) <lessOrEqual(0, 1)>: Bool =
  byte >= 97 && byte <= 122
