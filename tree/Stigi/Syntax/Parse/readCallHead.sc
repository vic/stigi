// prompt: read the type and capability arguments of a call or a handle and see whether value arguments follow.
def Stigi.Syntax.Parse.readCallHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.CallHead =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then CallHeadFailed(UnexpectedToken, token) else
    val typed: Stigi.Syntax.Parse.Listed = if Stigi.Syntax.Parse.kindAt(tables, token) is TokLeftBrack then Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readTypes(tables, Progress(token + 1, nodes, lists, pending), Brackets, Stigi.Limit.maxParseDepth)) else Stigi.Syntax.Parse.emptyList(start)
    typed match
      case ListFailed(fault, at) => CallHeadFailed(fault, at)
      case Listed(typeArgs, afterTypes) =>
        val Progress(capToken, typeNodes, typeLists, typePending) = afterTypes
        val capped: Stigi.Syntax.Parse.Listed = if Stigi.Syntax.Parse.kindAt(tables, capToken) is TokLeftBrace && capToken < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readNames(tables, Progress(capToken + 1, typeNodes, typeLists, typePending), Braces)) else Stigi.Syntax.Parse.emptyList(afterTypes)
        capped match
          case ListFailed(fault, at) => CallHeadFailed(fault, at)
          case Listed(capArgs, afterCaps) =>
            val Progress(parenToken, _, _, _) = afterCaps
            CallHead(typeArgs, capArgs, Stigi.Syntax.Parse.kindAt(tables, parenToken) is TokLeftParen, afterCaps)
