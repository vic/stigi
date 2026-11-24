// prompt: the first gathering state of a list: complete when the closing token is next.
def Stigi.Syntax.Parse.startGather(tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, bracket: Stigi.Syntax.Parse.Bracket) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Gather =
  val Progress(token, _, _, _) = start
  if Stigi.Syntax.Parse.closes(Stigi.Syntax.Parse.kindAt(tables, token), bracket) then Gathered(start) else Gathering(start)
