// prompt: read a fact group in angle brackets.
def Stigi.Syntax.Parse.readFactGroup {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token || !(Stigi.Syntax.Parse.kindAt(tables, token) is TokLess) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readFacts(tables, Progress(token + 1, nodes, lists, pending), Angles)) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(items, after) =>
        Stigi.Syntax.Parse.freeSlot(after) match
          case TableFull => Failed(UnitTooLarge, token)
          case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, FactGroup(at, items), after)
