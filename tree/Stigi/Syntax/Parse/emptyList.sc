// prompt: an empty list at the next free entry of the list table.
def Stigi.Syntax.Parse.emptyList(progress: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, lists, _) = progress
  if Stigi.Limit.maxUnitNodes < lists then ListFailed(UnitTooLarge, token) else Listed(Span(lists, lists), progress)
