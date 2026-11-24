// prompt: push one parsed alternative on the pending table and say whether another bar follows it.
def Stigi.Syntax.Parse.pushAlternative {Stigi.Mut} (tables: Stigi.Syntax.Tables, built: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Gather =
  built match
    case Failed(fault, at) => GatherFailed(fault, at)
    case Built(item, after) =>
      val Progress(next, nodes, lists, pending) = after
      val Tables(_, _, _, _, pendingTable, _) = tables
      if Stigi.Limit.maxUnitNodes <= pending then GatherFailed(UnitTooLarge, next) else
        Stigi.Syntax.Parse.storePending(pendingTable, pending, item)
        if Stigi.Syntax.Parse.kindAt(tables, next) is TokBar then Gathering(Progress(next, nodes, lists, pending + 1)) else Gathered(Progress(next, nodes, lists, pending + 1))
