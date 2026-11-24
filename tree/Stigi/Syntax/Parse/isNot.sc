// prompt: whether one token is the word not that negates a fact.
def Stigi.Syntax.Parse.isNot(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  val Token(kind, startPos, endPos, _, _) = Stigi.Syntax.Parse.tokenAt(tables, token)
  val Tables(source, _, _, _, _, _) = tables
  if !(kind is TokIdent) || Stigi.Limit.maxUnitBytes < endPos then false else Stigi.Lex.sliceEquals(Stigi.Limit.maxUnitBytes, source, startPos, endPos, 3, "not")
