// prompt: place a literal pattern: an integer with an optional minus, a byte, a string, true or false.
def Stigi.Syntax.Parse.patternLiteral {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    val last: U64 = if Stigi.Syntax.Parse.kindAt(tables, token) is TokMinus then token + 1 else token
    val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, last)
    if Stigi.Limit.maxUnitBytes <= last || !(kind is TokInt || kind is TokChar || kind is TokStr || kind is TokTrue || kind is TokFalse) then Failed(UnexpectedToken, token) else
      Stigi.Syntax.Parse.freeSlot(start) match
        case TableFull => Failed(UnitTooLarge, token)
        case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, PatLiteral(at, Stigi.Syntax.Parse.spanText(tables, token, last)), Progress(last + 1, nodes, lists, pending))
