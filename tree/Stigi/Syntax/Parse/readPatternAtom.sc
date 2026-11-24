// prompt: read one pattern atom: a tuple, a constructor with parts, a name, a literal or a literal range.
def Stigi.Syntax.Parse.readPatternAtom {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if depth < 1 then Failed(TooDeep, token) else if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    if Stigi.Syntax.Parse.kindAt(tables, token) is TokLeftParen then
      Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readPatterns(tables, Progress(token + 1, nodes, lists, pending), Parens, depth - 1)) match
        case ListFailed(fault, at) => Failed(fault, at)
        case Listed(parts, after) =>
          Stigi.Syntax.Parse.freeSlot(after) match
            case TableFull => Failed(UnitTooLarge, token)
            case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, PatTuple(at, parts), after)
    else if Stigi.Syntax.Parse.isWord(tables, token) then
      val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
      if Stigi.Limit.maxUnitBytes <= last then Failed(UnexpectedToken, last) else if Stigi.Syntax.Parse.kindAt(tables, last + 1) is TokLeftParen then
        Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.readPatterns(tables, Progress(last + 2, nodes, lists, pending), Parens, depth - 1)) match
          case ListFailed(fault, at) => Failed(fault, at)
          case Listed(parts, after) =>
            Stigi.Syntax.Parse.freeSlot(after) match
              case TableFull => Failed(UnitTooLarge, token)
              case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, PatCtor(at, Stigi.Syntax.Parse.spanText(tables, token, last), parts), after)
      else
        Stigi.Syntax.Parse.freeSlot(start) match
          case TableFull => Failed(UnitTooLarge, token)
          case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, PatName(at, Stigi.Syntax.Parse.spanText(tables, token, last)), Progress(last + 1, nodes, lists, pending))
    else
      Stigi.Syntax.Parse.patternLiteral(tables, start) match
        case Failed(fault, at) => Failed(fault, at)
        case Built(low, afterLow) =>
          val Progress(dots, rangeNodes, rangeLists, rangePending) = afterLow
          if Stigi.Limit.maxUnitBytes <= dots || !(Stigi.Syntax.Parse.kindAt(tables, dots) is TokDotDot) then Built(low, afterLow) else
            Stigi.Syntax.Parse.patternLiteral(tables, Progress(dots + 1, rangeNodes, rangeLists, rangePending)) match
              case Failed(fault, at) => Failed(fault, at)
              case Built(high, afterHigh) =>
                Stigi.Syntax.Parse.claimOver(afterHigh, low, high, high) match
                  case Unclaimed(fault) => Failed(fault, dots)
                  case Claimed(at, lowEnd, highEnd, _) => Stigi.Syntax.Parse.place(tables, at, PatRange(at, lowEnd, highEnd), afterHigh)
