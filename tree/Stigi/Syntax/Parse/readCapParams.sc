// prompt: read a list of capability parameters up to a closing token.
def Stigi.Syntax.Parse.readCapParams {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, bracket: Stigi.Syntax.Parse.Bracket) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  var state: Stigi.Syntax.Parse.Gather = Stigi.Syntax.Parse.startGather(tables, start, bracket)
  var steps: U64 = 0
  while steps < Stigi.Limit.maxUnitBytes && state is Gathering(_) do
    state = state match
      case Gathering(current) => Stigi.Syntax.Parse.pushItem(tables, bracket, Stigi.Syntax.Parse.readCapParam(tables, current))
      case Gathered(done) => Gathered(done)
      case GatherFailed(fault, at) => GatherFailed(fault, at)
    steps = steps + 1
  Stigi.Syntax.Parse.finishGather(tables, base, state, token)
