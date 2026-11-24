// prompt: read one group of a def head: a capability group in braces or a fact group in angle brackets.
def Stigi.Syntax.Parse.readGroup {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if Stigi.Syntax.Parse.kindAt(tables, token) is TokLess then Stigi.Syntax.Parse.readFactGroup(tables, start) else
    Stigi.Syntax.Parse.readCapGroup(tables, start) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(caps, after) =>
        Stigi.Syntax.Parse.freeSlot(after) match
          case TableFull => Failed(UnitTooLarge, token)
          case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, CapGroup(at, caps), after)
