// prompt: read the value parameters in parentheses at a token, or an empty list.
def Stigi.Syntax.Parse.readValueGroup {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, nodes, lists, pending) = start
  if !(Stigi.Syntax.Parse.kindAt(tables, token) is TokLeftParen) then Stigi.Syntax.Parse.emptyList(start) else Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readParams(tables, Progress(Stigi.Syntax.Parse.nextToken(token), nodes, lists, pending), Parens))
