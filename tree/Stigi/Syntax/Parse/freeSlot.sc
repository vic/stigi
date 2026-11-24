// prompt: the next free entry of the node table, or a full table.
def Stigi.Syntax.Parse.freeSlot(progress: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Slot =
  val Progress(_, nodes, _, _) = progress
  if nodes < Stigi.Limit.maxUnitNodes then FreeSlot(nodes) else TableFull
