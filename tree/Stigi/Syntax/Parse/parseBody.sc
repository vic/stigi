// prompt: parse a body after a separator: a block of statement lines after a line break, or one statement or control expression on the same line.
pub def Stigi.Syntax.Parse.parseBody {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, base) = start
  val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, token)
  if depth < 1 then Failed(TooDeep, token) else if kind is TokIf || kind is TokWhile then Stigi.Syntax.Parse.parseControl(tables, start, indent, depth - 1) else if !(kind is TokNewline) then Stigi.Syntax.Parse.parseLine(tables, start, indent, depth - 1) else
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
          if Stigi.Limit.maxUnitBytes <= first || Stigi.Syntax.Parse.kindAt(tables, first) is TokEnd || Stigi.Syntax.Parse.columnAt(tables, first) != column then Gathered(current) else Stigi.Syntax.Parse.pushLine(tables, Stigi.Syntax.Parse.parseLine(tables, Progress(first, lineNodes, lineLists, linePending), column, depth - 1))
      steps = steps + 1
    Stigi.Syntax.Parse.joinBlock(tables, Stigi.Syntax.Parse.finishGather(tables, base, state, token), token)
