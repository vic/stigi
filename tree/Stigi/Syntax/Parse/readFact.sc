// prompt: read one fact: a denial of a list of facts, or a fact without a denial.
def Stigi.Syntax.Parse.readFact {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token || !Stigi.Syntax.Parse.isNot(tables, token) || !(Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokLeftParen) then Stigi.Syntax.Parse.readPlainFact(tables, start) else
    Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readPlainFacts(tables, Progress(token + 2, nodes, lists, pending), Parens)) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(items, after) =>
        Stigi.Syntax.Parse.freeSlot(after) match
          case TableFull => Failed(UnitTooLarge, token)
          case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, FactDeny(at, items), after)
