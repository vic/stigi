// prompt: parse a control expression: a conditional, a loop or a match over a tuple of scrutinees.
def Stigi.Syntax.Parse.parseControl {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if depth < 1 then Failed(TooDeep, token) else if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    if Stigi.Syntax.Parse.kindAt(tables, token) is TokLeftParen then
      Stigi.Syntax.Parse.passCloser(Stigi.Syntax.Parse.parseArgs(tables, Progress(token + 1, nodes, lists, pending), Parens, indent, depth - 1)) match
        case ListFailed(fault, at) => Failed(fault, at)
        case Listed(scrutinees, after) =>
          val Progress(matchToken, matchNodes, matchLists, matchPending) = after
          if Stigi.Limit.maxUnitBytes <= matchToken then Failed(UnexpectedToken, matchToken) else Stigi.Syntax.Parse.joinMatch(tables, scrutinees, Stigi.Syntax.Parse.parseArms(tables, Progress(matchToken + 1, matchNodes, matchLists, matchPending), depth - 1), matchToken)
    else
      Stigi.Syntax.Parse.parseExpr(tables, Progress(token + 1, nodes, lists, pending), 0, indent, depth - 1) match
        case Failed(fault, at) => Failed(fault, at)
        case Built(condition, afterCondition) =>
          val Progress(keyword, conditionNodes, conditionLists, conditionPending) = afterCondition
          if Stigi.Limit.maxUnitBytes <= keyword then Failed(UnexpectedToken, keyword) else if Stigi.Syntax.Parse.kindAt(tables, token) is TokWhile then
            if !(Stigi.Syntax.Parse.kindAt(tables, keyword) is TokLoopBody) then Failed(UnexpectedToken, keyword) else Stigi.Syntax.Parse.joinWhile(tables, condition, Stigi.Syntax.Parse.parseBody(tables, Progress(keyword + 1, conditionNodes, conditionLists, conditionPending), indent, depth - 1), token)
          else if !(Stigi.Syntax.Parse.kindAt(tables, keyword) is TokThen) then Failed(UnexpectedToken, keyword) else
            Stigi.Syntax.Parse.parseBody(tables, Progress(keyword + 1, conditionNodes, conditionLists, conditionPending), indent, depth - 1) match
              case Failed(fault, at) => Failed(fault, at)
              case Built(whenTrue, afterTrue) =>
                val Progress(search, trueNodes, trueLists, truePending) = afterTrue
                Stigi.Syntax.Parse.elseAfter(tables, search, indent) match
                  case ElseMissing => Failed(UnexpectedToken, search)
                  case ElseFound(elseToken) => if Stigi.Limit.maxUnitBytes <= elseToken then Failed(UnexpectedToken, elseToken) else Stigi.Syntax.Parse.joinIf(tables, condition, whenTrue, Stigi.Syntax.Parse.parseBody(tables, Progress(elseToken + 1, trueNodes, trueLists, truePending), indent, depth - 1), token)
