// prompt: parse one match arm on a line: case, a pattern, an optional guard, the arrow and a body.
def Stigi.Syntax.Parse.parseArm {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, column: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(first, lineNodes, lineLists, linePending) = start
  if depth < 1 then Failed(TooDeep, first) else if Stigi.Limit.maxUnitBytes <= first then Failed(UnexpectedToken, first) else
    Stigi.Syntax.Parse.readPattern(tables, Progress(first + 1, lineNodes, lineLists, linePending), Stigi.Limit.maxParseDepth) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(pattern, afterPattern) =>
        val Progress(guardToken, patternNodes, patternLists, patternPending) = afterPattern
        val guarded: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.kindAt(tables, guardToken) is TokIf && guardToken < Stigi.Limit.maxUnitBytes then Stigi.Syntax.Parse.parseExpr(tables, Progress(guardToken + 1, patternNodes, patternLists, patternPending), 0, column, depth - 1) else Stigi.Syntax.Parse.absent(tables, afterPattern)
        guarded match
          case Failed(fault, at) => Failed(fault, at)
          case Built(guard, afterGuard) =>
            val Progress(arrow, guardNodes, guardLists, guardPending) = afterGuard
            if Stigi.Limit.maxUnitBytes <= arrow || !(Stigi.Syntax.Parse.kindAt(tables, arrow) is TokArrow) then Failed(UnexpectedToken, arrow) else Stigi.Syntax.Parse.joinArm(tables, pattern, guard, Stigi.Syntax.Parse.parseBody(tables, Progress(arrow + 1, guardNodes, guardLists, guardPending), column, depth - 1), first)
