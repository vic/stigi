// prompt: claim the next free entry of the node table above up to three earlier child nodes.
def Stigi.Syntax.Parse.claimOver(progress: Stigi.Syntax.Parse.Progress, first: U64, second: U64, third: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Claim =
  val Progress(_, nodes, _, _) = progress
  if Stigi.Limit.maxUnitNodes <= nodes then Unclaimed(UnitTooLarge) else if nodes <= first || nodes <= second || nodes <= third then Unclaimed(ChildAfterParent) else Claimed(nodes, first, second, third)
