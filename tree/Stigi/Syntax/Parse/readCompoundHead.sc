// prompt: read the head of a capability, handler, trait or given from its name token up to the colon before its members.
def Stigi.Syntax.Parse.readCompoundHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, kind: Stigi.Syntax.Parse.CompoundKind) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.CompoundHead =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if !Stigi.Syntax.Parse.isWord(tables, token) then CompoundHeadFailed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readTypeParams(tables, Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)) match
      case ListFailed(fault, at) => CompoundHeadFailed(fault, at)
      case Listed(typeParams, afterTypes) =>
        Stigi.Syntax.Parse.readCapGroup(tables, afterTypes) match
          case ListFailed(fault, at) => CompoundHeadFailed(fault, at)
          case Listed(capParams, afterCaps) =>
            val Progress(paren, _, _, _) = afterCaps
            Stigi.Syntax.Parse.readValueGroup(tables, afterCaps) match
              case ListFailed(fault, at) => CompoundHeadFailed(fault, at)
              case Listed(valueParams, afterValues) =>
                Stigi.Syntax.Parse.readTarget(tables, afterValues, kind) match
                  case Failed(fault, at) => CompoundHeadFailed(fault, at)
                  case Built(target, afterTarget) =>
                    val Progress(colon, targetNodes, targetLists, targetPending) = afterTarget
                    if !(Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon) then CompoundHeadFailed(UnexpectedToken, colon) else CompoundHead(Stigi.Syntax.Parse.spanText(tables, token, last), typeParams, capParams, Stigi.Syntax.Parse.kindAt(tables, paren) is TokLeftParen, valueParams, target, Progress(Stigi.Syntax.Parse.nextToken(colon), targetNodes, targetLists, targetPending))
