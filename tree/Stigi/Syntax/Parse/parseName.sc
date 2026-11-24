// prompt: place a name node for the dotted name at the next token.
def Stigi.Syntax.Parse.parseName {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if !Stigi.Syntax.Parse.isWord(tables, token) || Stigi.Limit.maxUnitBytes <= last then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.freeSlot(start) match
      case TableFull => Failed(UnitTooLarge, token)
      case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, Name(at, Stigi.Syntax.Parse.spanText(tables, token, last)), Progress(last + 1, nodes, lists, pending))
