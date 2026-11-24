// prompt: read the head of a val statement: its pattern, an optional type and the equals sign.
def Stigi.Syntax.Parse.readValHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Head =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then HeadFailed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readPattern(tables, Progress(token + 1, nodes, lists, pending), Stigi.Limit.maxParseDepth) match
      case Failed(fault, at) => HeadFailed(fault, at)
      case Built(pattern, afterPattern) =>
        val Progress(colon, patternNodes, patternLists, patternPending) = afterPattern
        val typed: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon && colon < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.readType(tables, Progress(colon + 1, patternNodes, patternLists, patternPending), Stigi.Limit.maxParseDepth) else Stigi.Syntax.Parse.absent(tables, afterPattern)
        typed match
          case Failed(fault, at) => HeadFailed(fault, at)
          case Built(declared, afterType) =>
            val Progress(equals, typeNodes, typeLists, typePending) = afterType
            if Stigi.Limit.maxUnitBytes <= equals || !(Stigi.Syntax.Parse.kindAt(tables, equals) is TokAssign) then HeadFailed(UnexpectedToken, equals) else ValHead(pattern, declared, Progress(equals + 1, typeNodes, typeLists, typePending))
