// prompt: place a literal node when a literal token is next.
def Stigi.Syntax.Parse.literalAt {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  Stigi.Syntax.Parse.leadOf(Stigi.Syntax.Parse.kindAt(tables, token)) match
    case StartsLiteral(kind) => Stigi.Syntax.Parse.literal(tables, start, kind, token)
    case StartsName | StartsParen | StartsPrefix(_) | StartsIf | StartsWhile | StartsVal | StartsVar | StartsHandle | StartsAssert | StartsDecl | StartsNothing => Failed(UnexpectedToken, token)
