// prompt: read the result type after the colon of a def head, or an absent node when exit facts follow the colon.
def Stigi.Syntax.Parse.readResult {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if Stigi.Syntax.Parse.kindAt(tables, token) is TokLess then Stigi.Syntax.Parse.absent(tables, start) else Stigi.Syntax.Parse.readType(tables, start, Stigi.Limit.maxParseDepth)
