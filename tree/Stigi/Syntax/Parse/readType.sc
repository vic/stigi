// prompt: read a type: a dotted name, optional type arguments in brackets, length arguments in parentheses and a capability in braces.
def Stigi.Syntax.Parse.readType {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if depth < 1 then Failed(TooDeep, token) else
    val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
    if !Stigi.Syntax.Parse.isWord(tables, token) || Stigi.Limit.maxUnitBytes <= last then Failed(UnexpectedToken, token) else
      val afterName: Stigi.Syntax.Parse.Progress = Progress(last + 1, nodes, lists, pending)
      val typed: Stigi.Syntax.Parse.Listed = if Stigi.Syntax.Parse.kindAt(tables, last + 1) is TokLeftBrack then Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readTypes(tables, Progress(last + 2, nodes, lists, pending), Brackets, depth - 1)) else Stigi.Syntax.Parse.emptyList(afterName)
      typed match
        case ListFailed(fault, at) => Failed(fault, at)
        case Listed(typeArgs, afterTypes) =>
          val Progress(lengthToken, typeNodes, typeLists, typePending) = afterTypes
          val sized: Stigi.Syntax.Parse.Listed = if Stigi.Syntax.Parse.kindAt(tables, lengthToken) is TokLeftParen && lengthToken < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readTerms(tables, Progress(lengthToken + 1, typeNodes, typeLists, typePending), Parens)) else Stigi.Syntax.Parse.emptyList(afterTypes)
          sized match
            case ListFailed(fault, at) => Failed(fault, at)
            case Listed(lengthArgs, afterLengths) =>
              Stigi.Syntax.Parse.capArgument(tables, afterLengths) match
                case Failed(fault, at) => Failed(fault, at)
                case Built(capArg, afterCap) =>
                  Stigi.Syntax.Parse.claimOver(afterCap, capArg, capArg, capArg) match
                    case Unclaimed(fault) => Failed(fault, last)
                    case Claimed(at, cap, _, _) => Stigi.Syntax.Parse.place(tables, at, TypeRef(at, Stigi.Syntax.Parse.spanText(tables, token, last), typeArgs, lengthArgs, cap), afterCap)
