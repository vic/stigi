// prompt: check whether an ASCII byte continues an identifier or dotted name in L0 source.
def Stigi.Emit.isNameBodyByte(byteVal: U8) <lessOrEqual(0, 1)>: Bool =
  Stigi.Lex.isLetterByte(byteVal) || Stigi.Lex.isDigitByte(byteVal) || byteVal == 95 || byteVal == 46
