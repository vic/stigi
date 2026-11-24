// prompt: read the type parameters in brackets at a token, or an empty list.
def Stigi.Syntax.Parse.readTypeParams {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, nodes, lists, pending) = start
  if !(Stigi.Syntax.Parse.kindAt(tables, token) is TokLeftBrack) then Stigi.Syntax.Parse.emptyList(start) else Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readParams(tables, Progress(Stigi.Syntax.Parse.nextToken(token), nodes, lists, pending), Brackets))
