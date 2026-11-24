// prompt: write one item into one entry of the pending table.
def Stigi.Syntax.Parse.storePending {Stigi.Mut} (pending: Array[Stigi.Syntax.NodeAt](Stigi.Limit.maxUnitNodes), slot: U64, item: Stigi.Syntax.NodeAt) <below(slot, Stigi.Limit.maxUnitNodes)>: Unit =
  pending(slot) := item
