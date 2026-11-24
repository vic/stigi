// prompt: place an absent node for an optional part that the source leaves out.
def Stigi.Syntax.Parse.absent {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  Stigi.Syntax.Parse.freeSlot(start) match
    case TableFull => Failed(UnitTooLarge, token)
    case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, Absent(at), start)
