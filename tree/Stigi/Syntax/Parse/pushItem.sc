// prompt: push one parsed item on the pending table and say whether a comma, the closing token or a fault follows it.
def Stigi.Syntax.Parse.pushItem {Stigi.Mut} (tables: Stigi.Syntax.Tables, bracket: Stigi.Syntax.Parse.Bracket, built: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Gather =
  built match
    case Failed(fault, at) => GatherFailed(fault, at)
    case Built(item, after) =>
      val Progress(next, nodes, lists, pending) = after
      val Tables(_, _, _, _, pendingTable, _) = tables
      if Stigi.Limit.maxUnitNodes <= pending || Stigi.Limit.maxUnitBytes <= next then GatherFailed(UnitTooLarge, next) else
        Stigi.Syntax.Parse.storePending(pendingTable, pending, item)
        if Stigi.Syntax.Parse.kindAt(tables, next) is TokComma then Gathering(Progress(next + 1, nodes, lists, pending + 1)) else if Stigi.Syntax.Parse.closes(Stigi.Syntax.Parse.kindAt(tables, next), bracket) then Gathered(Progress(next, nodes, lists, pending + 1)) else GatherFailed(UnexpectedToken, next)
