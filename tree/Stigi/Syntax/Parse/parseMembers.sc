// prompt: parse the member lines of one kind after a line break, at the column of the first of them.
def Stigi.Syntax.Parse.parseMembers {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, kind: Stigi.Syntax.Parse.MemberKind, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  if depth < 1 then ListFailed(TooDeep, token) else if !(Stigi.Syntax.Parse.kindAt(tables, token) is TokNewline) then Stigi.Syntax.Parse.emptyList(start) else
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
          val firstKind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, first)
          val fits: Bool = if kind is VarMembers then firstKind is TokVar else Stigi.Syntax.Parse.leadOf(firstKind) is StartsDecl
          if !fits || Stigi.Syntax.Parse.columnAt(tables, first) != column then Gathered(current) else
            val lineStart: Stigi.Syntax.Parse.Progress = Progress(first, lineNodes, lineLists, linePending)
            Stigi.Syntax.Parse.pushLine(tables, if kind is VarMembers then Stigi.Syntax.Parse.parseLine(tables, lineStart, column, depth - 1) else Stigi.Syntax.Parse.parseDecl(tables, lineStart, column, depth - 1))
      steps = steps + 1
    Stigi.Syntax.Parse.finishGather(tables, base, state, token)
