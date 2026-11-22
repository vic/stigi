// prompt: check whether one byte is an ASCII decimal digit.
pub def Stigi.Lex.isDigitByte(byte: U8) <lessOrEqual(0, 1)>: Bool =
  byte >= 48 && byte <= 57
