// prompt: read a type declaration from its name token: name, type, value and capability parameters, the target type and optional facts.
def Stigi.Syntax.Parse.readTypeDecl {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if !Stigi.Syntax.Parse.isWord(tables, token) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readTypeParams(tables, Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(typeParams, afterTypes) =>
        Stigi.Syntax.Parse.readValueGroup(tables, afterTypes) match
          case ListFailed(fault, at) => Failed(fault, at)
          case Listed(valueParams, afterValues) =>
            Stigi.Syntax.Parse.readCapGroup(tables, afterValues) match
              case ListFailed(fault, at) => Failed(fault, at)
              case Listed(capParams, afterCaps) =>
                val Progress(equals, capNodes, capLists, capPending) = afterCaps
                if !(Stigi.Syntax.Parse.kindAt(tables, equals) is TokAssign) then Failed(UnexpectedToken, equals) else
                  Stigi.Syntax.Parse.readType(tables, Progress(Stigi.Syntax.Parse.nextToken(equals), capNodes, capLists, capPending), Stigi.Limit.maxParseDepth) match
                    case Failed(fault, at) => Failed(fault, at)
                    case Built(target, afterTarget) =>
                      val Progress(factToken, _, _, _) = afterTarget
                      val held: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.kindAt(tables, factToken) is TokLess then Stigi.Syntax.Parse.readFactGroup(tables, afterTarget) else Stigi.Syntax.Parse.absent(tables, afterTarget)
                      held match
                        case Failed(fault, at) => Failed(fault, at)
                        case Built(facts, after) =>
                          Stigi.Syntax.Parse.claimOver(after, target, facts, facts) match
                            case Unclaimed(fault) => Failed(fault, token)
                            case Claimed(at, aliased, stated, _) => Stigi.Syntax.Parse.place(tables, at, TypeDecl(at, isPublic, Stigi.Syntax.Parse.spanText(tables, token, last), typeParams, valueParams, capParams, aliased, stated), after)
