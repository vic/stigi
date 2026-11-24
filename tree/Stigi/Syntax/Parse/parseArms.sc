// prompt: parse the match arms on the lines after a line break, at the column of the first arm.
def Stigi.Syntax.Parse.parseArms {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  if depth < 1 then ListFailed(TooDeep, token) else if !(Stigi.Syntax.Parse.kindAt(tables, token) is TokNewline) then ListFailed(UnexpectedToken, token) else
    val column: U64 = Stigi.Syntax.Parse.columnAt(tables, Stigi.Syntax.Parse.lineStart(tables, token, Stigi.Limit.maxUnitBytes))
    var state: Stigi.Syntax.Parse.Gather = Gathering(start)
    var steps: U64 = 0
    while steps < Stigi.Limit.maxUnitBytes && state is Gathering(_) do
      state = state match
        case Gathered(done) => Gathered(done)
        case GatherFailed(fault, at) => GatherFailed(fault, at)
        case Gathering(current) =>
          val Progress(lineToken, lineNodes, lineLists, linePending) = current
          val first: U64 = Stigi.Syntax.Parse.lineStart(tables, lineToken, Stigi.Limit.maxUnitBytes)
          if Stigi.Limit.maxUnitBytes <= first || !(Stigi.Syntax.Parse.kindAt(tables, first) is TokCase) || Stigi.Syntax.Parse.columnAt(tables, first) != column then Gathered(current) else Stigi.Syntax.Parse.pushLine(tables, Stigi.Syntax.Parse.parseArm(tables, Progress(first, lineNodes, lineLists, linePending), column, depth - 1))
      steps = steps + 1
    Stigi.Syntax.Parse.finishGather(tables, base, state, token)
