// prompt: check whether one byte is an ASCII letter.
pub def Stigi.Lex.isLetterByte(byte: U8) <lessOrEqual(0, 1)>: Bool =
  Stigi.Lex.isUpperByte(byte) || Stigi.Lex.isLowerByte(byte)
