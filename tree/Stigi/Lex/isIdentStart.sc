// prompt: test whether an ASCII byte can start a Stigi identifier.
def Stigi.Lex.isIdentStart(byte: U8): Bool =
  byte >= 97 && byte <= 122 || byte >= 65 && byte <= 90 || byte == 95
