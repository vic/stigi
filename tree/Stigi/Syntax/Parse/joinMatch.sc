// prompt: join scrutinees and parsed arms into a match.
def Stigi.Syntax.Parse.joinMatch {Stigi.Mut} (tables: Stigi.Syntax.Tables, scrutinees: Stigi.Syntax.Span, arms: Stigi.Syntax.Parse.Listed, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  arms match
    case ListFailed(fault, at) => Failed(fault, at)
    case Listed(cases, after) =>
      Stigi.Syntax.Parse.freeSlot(after) match
        case TableFull => Failed(UnitTooLarge, token)
        case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, MatchOn(at, scrutinees, cases), after)
