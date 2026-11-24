// prompt: read one token of the token table; a token past the table reads as the end token.
pub def Stigi.Syntax.Parse.tokenAt(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Lex.Token =
  val Tables(_, tokens, _, _, _, _) = tables
  if token < Stigi.Limit.maxUnitBytes then tokens(token) else Token(TokEnd, 0, 0, 0, 0)
