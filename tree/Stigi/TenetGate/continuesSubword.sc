// prompt: check whether one byte stays inside the current lowercase subword.
def Stigi.TenetGate.continuesSubword(byte: U8) <lessOrEqual(0, 1)>: Bool =
  byte >= 97 && byte <= 122
