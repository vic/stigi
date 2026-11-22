// prompt: check whether an ASCII byte can be part of a C identifier.
def Stigi.Emit.isCIdentByte(byteVal: U8) <lessOrEqual(0, 1)>: Bool =
  Stigi.Lex.isLetterByte(byteVal) || Stigi.Lex.isDigitByte(byteVal) || byteVal == 95
