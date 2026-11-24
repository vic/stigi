// prompt: copy the first bytes of a text buffer into the source table and lex them into the token table.
def Stigi.Syntax.Parse.lexBuffer {Stigi.Mut} (tables: Stigi.Syntax.Tables, textCap: U64, text: Array[U8](textCap), size: U64) <lessOrEqual(size, textCap)>: Stigi.Lex.LexResult =
  val Tables(source, tokens, _, _, _, _) = tables
  val copied: U64 = Stigi.Array.copy[U8](textCap, text, 0, size, Stigi.Limit.maxUnitBytes, source, 0)
  Stigi.Lex.scanAll(Stigi.Limit.maxUnitBytes, source, copied, Stigi.Limit.maxUnitBytes, tokens)
