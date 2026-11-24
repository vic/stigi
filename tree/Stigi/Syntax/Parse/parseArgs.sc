// prompt: parse a list of expressions up to a closing token.
def Stigi.Syntax.Parse.parseArgs {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, bracket: Stigi.Syntax.Parse.Bracket, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  if depth < 1 then ListFailed(TooDeep, token) else
    var state: Stigi.Syntax.Parse.Gather = Stigi.Syntax.Parse.startGather(tables, start, bracket)
    var steps: U64 = 0
    while steps < Stigi.Limit.maxUnitBytes && state is Gathering(_) do
      state = state match
        case Gathering(current) => Stigi.Syntax.Parse.pushItem(tables, bracket, Stigi.Syntax.Parse.parseExpr(tables, current, 0, indent, depth - 1))
        case Gathered(done) => Gathered(done)
        case GatherFailed(fault, at) => GatherFailed(fault, at)
      steps = steps + 1
    Stigi.Syntax.Parse.finishGather(tables, base, state, token)
