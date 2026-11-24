// prompt: read the head of a val declaration from its name token: its name, optional capabilities, the colon, its type and the equals sign.
def Stigi.Syntax.Parse.readValDeclHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.ValDeclHead =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if !Stigi.Syntax.Parse.isWord(tables, token) then ValDeclHeadFailed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readCapGroup(tables, Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)) match
      case ListFailed(fault, at) => ValDeclHeadFailed(fault, at)
      case Listed(caps, afterCaps) =>
        val Progress(colon, capNodes, capLists, capPending) = afterCaps
        if !(Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon) then ValDeclHeadFailed(UnexpectedToken, colon) else
          Stigi.Syntax.Parse.readType(tables, Progress(Stigi.Syntax.Parse.nextToken(colon), capNodes, capLists, capPending), Stigi.Limit.maxParseDepth) match
            case Failed(fault, at) => ValDeclHeadFailed(fault, at)
            case Built(declared, afterType) =>
              val Progress(equals, typeNodes, typeLists, typePending) = afterType
              if !(Stigi.Syntax.Parse.kindAt(tables, equals) is TokAssign) then ValDeclHeadFailed(UnexpectedToken, equals) else ValDeclHead(Stigi.Syntax.Parse.spanText(tables, token, last), caps, declared, Progress(Stigi.Syntax.Parse.nextToken(equals), typeNodes, typeLists, typePending))
