// prompt: read a def head after its name: the front, the facts after the value parameters, the colon, the result type and the exit facts.
def Stigi.Syntax.Parse.readHead {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  Stigi.Syntax.Parse.readHeadFront(tables, start) match
    case HeadFrontFailed(fault, at) => Failed(fault, at)
    case HeadFront(typeParams, preGroups, valueGroup, params, afterFront) =>
      Stigi.Syntax.Parse.readGroups(tables, afterFront) match
        case ListFailed(fault, at) => Failed(fault, at)
        case Listed(postFacts, afterPost) =>
          val Progress(colon, postNodes, postLists, postPending) = afterPost
          if !(Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon) then Failed(UnexpectedToken, colon) else
            Stigi.Syntax.Parse.readResult(tables, Progress(Stigi.Syntax.Parse.nextToken(colon), postNodes, postLists, postPending)) match
              case Failed(fault, at) => Failed(fault, at)
              case Built(returnType, afterResult) =>
                Stigi.Syntax.Parse.readGroups(tables, afterResult) match
                  case ListFailed(fault, at) => Failed(fault, at)
                  case Listed(exitFacts, after) =>
                    Stigi.Syntax.Parse.claimOver(after, returnType, returnType, returnType) match
                      case Unclaimed(fault) => Failed(fault, colon)
                      case Claimed(at, returned, _, _) => Stigi.Syntax.Parse.place(tables, at, DefHead(at, typeParams, preGroups, valueGroup, params, postFacts, returned, exitFacts), after)
