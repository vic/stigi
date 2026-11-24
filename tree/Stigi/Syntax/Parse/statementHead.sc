// prompt: read the head of one statement line by its first token.
def Stigi.Syntax.Parse.statementHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Head =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then HeadFailed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.leadOf(Stigi.Syntax.Parse.kindAt(tables, token)) match
      case StartsVal => Stigi.Syntax.Parse.readValHead(tables, start)
      case StartsVar => Stigi.Syntax.Parse.readVarHead(tables, start)
      case StartsHandle => Stigi.Syntax.Parse.readHandleHead(tables, start)
      case StartsName => if Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokAssign then AssignHead(Stigi.Syntax.Parse.spanText(tables, token, token), Progress(token + 2, nodes, lists, pending)) else ValueHead(start)
      case StartsLiteral(_) | StartsParen | StartsPrefix(_) | StartsIf | StartsWhile | StartsAssert | StartsNothing => ValueHead(start)
      case StartsDecl => HeadFailed(UnexpectedToken, token)
