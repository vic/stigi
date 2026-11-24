// prompt: read a bare assertion: an at sign and a fact group.
def Stigi.Syntax.Parse.bareAssert {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  Stigi.Syntax.Parse.absent(tables, start) match
    case Failed(fault, at) => Failed(fault, at)
    case Built(missing, afterMissing) =>
      val Progress(_, missingNodes, missingLists, missingPending) = afterMissing
      if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else Stigi.Syntax.Parse.joinAssert(tables, missing, Stigi.Syntax.Parse.readFactGroup(tables, Progress(token + 1, missingNodes, missingLists, missingPending)), token)
