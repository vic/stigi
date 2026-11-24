// prompt: copy a unit text into the source table and lex it into the token table.
pub def Stigi.Syntax.Parse.lexUnit {Stigi.Mut} (tables: Stigi.Syntax.Tables, size: U64, text: Array[U8](size)) <lessOrEqual(size, Stigi.Limit.maxUnitBytes)>: Stigi.Lex.LexResult =
  val Tables(source, tokens, _, _, _, _) = tables
  val copied: U64 = Stigi.Array.copy[U8](size, text, 0, size, Stigi.Limit.maxUnitBytes, source, 0)
  Stigi.Lex.scanAll(Stigi.Limit.maxUnitBytes, source, copied, Stigi.Limit.maxUnitBytes, tokens)
