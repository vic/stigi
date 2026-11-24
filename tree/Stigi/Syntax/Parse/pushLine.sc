// prompt: push one parsed line on the pending table when a line break or the end follows it.
def Stigi.Syntax.Parse.pushLine {Stigi.Mut} (tables: Stigi.Syntax.Tables, line: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Gather =
  line match
    case Failed(fault, at) => GatherFailed(fault, at)
    case Built(item, after) =>
      val Progress(next, nodes, lists, pending) = after
      val Tables(_, _, _, _, pendingTable, _) = tables
      val nextKind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, next)
      if Stigi.Limit.maxUnitNodes <= pending then GatherFailed(UnitTooLarge, next) else if !(nextKind is TokNewline || nextKind is TokEnd) then GatherFailed(UnexpectedToken, next) else
        Stigi.Syntax.Parse.storePending(pendingTable, pending, item)
        Gathering(Progress(next, nodes, lists, pending + 1))
