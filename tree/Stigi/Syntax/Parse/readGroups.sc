// prompt: read the capability and fact groups that follow, or an empty list.
def Stigi.Syntax.Parse.readGroups {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, token)
  if !(kind is TokLeftBrace || kind is TokLess) then Stigi.Syntax.Parse.emptyList(start) else
    var state: Stigi.Syntax.Parse.Gather = Gathering(start)
    var steps: U64 = 0
    while steps < Stigi.Limit.maxUnitBytes && state is Gathering(_) do
      state = state match
        case Gathering(current) => Stigi.Syntax.Parse.pushGroup(tables, Stigi.Syntax.Parse.readGroup(tables, current))
        case Gathered(done) => Gathered(done)
        case GatherFailed(fault, at) => GatherFailed(fault, at)
      steps = steps + 1
    Stigi.Syntax.Parse.finishGather(tables, base, state, token)
