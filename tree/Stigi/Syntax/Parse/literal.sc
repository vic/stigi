// prompt: place a literal node for the tokens from the next token to a last token.
def Stigi.Syntax.Parse.literal {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, kind: Stigi.Syntax.LiteralKind, last: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= last then Failed(UnexpectedToken, last) else
    Stigi.Syntax.Parse.freeSlot(start) match
      case TableFull => Failed(UnitTooLarge, token)
      case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, Literal(at, kind, Stigi.Syntax.Parse.spanText(tables, token, last)), Progress(last + 1, nodes, lists, pending))
