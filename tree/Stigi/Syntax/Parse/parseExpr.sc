// prompt: parse an expression: operands joined by binary operators and is tests that bind tighter than a floor, with the match arms that can follow it.
pub def Stigi.Syntax.Parse.parseExpr {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, floor: U64, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if depth < 1 then Failed(TooDeep, token) else
    var state: Stigi.Syntax.Parse.Built = Stigi.Syntax.Parse.parseOperand(tables, start, indent, depth - 1)
    var steps: U64 = 0
    while steps < Stigi.Limit.maxUnitBytes && Stigi.Syntax.Parse.continuesInfix(tables, state, floor) do
      state = state match
        case Failed(fault, at) => Failed(fault, at)
        case Built(left, progress) =>
          val Progress(operatorToken, nodes, lists, pending) = progress
          if Stigi.Limit.maxUnitBytes <= operatorToken then Failed(UnexpectedToken, operatorToken) else if Stigi.Syntax.Parse.kindAt(tables, operatorToken) is TokIs then Stigi.Syntax.Parse.joinIs(tables, left, Stigi.Syntax.Parse.readPattern(tables, Progress(operatorToken + 1, nodes, lists, pending), Stigi.Limit.maxParseDepth), operatorToken) else Stigi.Syntax.Parse.joinBinary(tables, left, operatorToken, Stigi.Syntax.Parse.parseExpr(tables, Progress(operatorToken + 1, nodes, lists, pending), Stigi.Syntax.Parse.bindingAt(tables, operatorToken), indent, depth - 1))
      steps = steps + 1
    if 0 < floor then state else
      state match
        case Failed(fault, at) => Failed(fault, at)
        case Built(value, after) =>
          val Progress(matchToken, _, _, _) = after
          if Stigi.Limit.maxUnitBytes <= matchToken || !(Stigi.Syntax.Parse.kindAt(tables, matchToken) is TokMatch) then Built(value, after) else
            Stigi.Syntax.Parse.singleList(tables, value, after) match
              case ListFailed(fault, at) => Failed(fault, at)
              case Listed(scrutinees, listed) =>
                val Progress(_, listedNodes, listedLists, listedPending) = listed
                Stigi.Syntax.Parse.joinMatch(tables, scrutinees, Stigi.Syntax.Parse.parseArms(tables, Progress(matchToken + 1, listedNodes, listedLists, listedPending), depth - 1), matchToken)
