// prompt: read the head of a var statement: its name, an optional invariant, an optional type and the equals sign.
def Stigi.Syntax.Parse.readVarHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Head =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token || !Stigi.Syntax.Parse.isWord(tables, token + 1) then HeadFailed(UnexpectedToken, token) else
    val held: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.kindAt(tables, token + 2) is TokAt then Stigi.Syntax.Parse.readFactGroup(tables, Progress(token + 3, nodes, lists, pending)) else Stigi.Syntax.Parse.absent(tables, Progress(token + 2, nodes, lists, pending))
    held match
      case Failed(fault, at) => HeadFailed(fault, at)
      case Built(invariant, afterInvariant) =>
        val Progress(colon, heldNodes, heldLists, heldPending) = afterInvariant
        val typed: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon && colon < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.readType(tables, Progress(colon + 1, heldNodes, heldLists, heldPending), Stigi.Limit.maxParseDepth) else Stigi.Syntax.Parse.absent(tables, afterInvariant)
        typed match
          case Failed(fault, at) => HeadFailed(fault, at)
          case Built(declared, afterType) =>
            val Progress(equals, typeNodes, typeLists, typePending) = afterType
            if Stigi.Limit.maxUnitBytes <= equals || !(Stigi.Syntax.Parse.kindAt(tables, equals) is TokAssign) then HeadFailed(UnexpectedToken, equals) else VarHead(Stigi.Syntax.Parse.spanText(tables, token + 1, token + 1), invariant, declared, Progress(equals + 1, typeNodes, typeLists, typePending))
