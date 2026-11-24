// prompt: read the enum case lines after a line break, at the column of the first case.
def Stigi.Syntax.Parse.readEnumCases {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  if !(Stigi.Syntax.Parse.kindAt(tables, token) is TokNewline) then ListFailed(UnexpectedToken, token) else
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
          if !(Stigi.Syntax.Parse.kindAt(tables, first) is TokCase) || Stigi.Syntax.Parse.columnAt(tables, first) != column then Gathered(current) else Stigi.Syntax.Parse.pushLine(tables, Stigi.Syntax.Parse.readEnumCase(tables, Progress(Stigi.Syntax.Parse.nextToken(first), lineNodes, lineLists, linePending)))
      steps = steps + 1
    Stigi.Syntax.Parse.finishGather(tables, base, state, token)
