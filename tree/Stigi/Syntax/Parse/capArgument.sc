// prompt: place the capability argument of a type in braces as a name node, or an absent node when there is none.
def Stigi.Syntax.Parse.capArgument {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if !(Stigi.Syntax.Parse.kindAt(tables, token) is TokLeftBrace) || Stigi.Limit.maxUnitBytes <= token then Stigi.Syntax.Parse.absent(tables, start) else
    val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token + 1, Stigi.Limit.maxUnitBytes)
    if Stigi.Limit.maxUnitBytes <= last || !Stigi.Syntax.Parse.isWord(tables, token + 1) || !(Stigi.Syntax.Parse.kindAt(tables, last + 1) is TokRightBrace) then Failed(UnexpectedToken, token) else
      Stigi.Syntax.Parse.freeSlot(start) match
        case TableFull => Failed(UnitTooLarge, token)
        case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, Name(at, Stigi.Syntax.Parse.spanText(tables, token + 1, last)), Progress(last + 2, nodes, lists, pending))
