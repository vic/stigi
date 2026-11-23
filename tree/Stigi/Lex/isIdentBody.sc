// prompt: test whether an ASCII byte can continue a Stigi identifier.
def Stigi.Lex.isIdentBody(byte: U8): Bool =
  byte >= 97 && byte <= 122 || byte >= 65 && byte <= 90 || byte >= 48 && byte <= 57 || byte == 95
