// prompt: read the alias lines that follow the prompt line of a unit into a list.
def Stigi.Syntax.Parse.readAliases {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes), below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, _, _, base) = start
  var gathering: Stigi.Syntax.Parse.Gather = Gathering(start)
  var steps: U64 = 0
  while steps < Stigi.Limit.maxUnitBytes && gathering is Gathering(_) do
    gathering = gathering match
      case Gathering(current) =>
        val Progress(lineToken, nodes, lists, pending) = current
        val lineAt: U64 = Stigi.Syntax.Parse.lineStart(tables, lineToken, Stigi.Limit.maxUnitBytes)
        if Stigi.Syntax.Parse.isAliasWord(tables, lineAt) then Stigi.Syntax.Parse.pushLine(tables, Stigi.Syntax.Parse.parseAlias(tables, Progress(lineAt, nodes, lists, pending))) else Gathered(Progress(lineAt, nodes, lists, pending))
      case Gathered(done) => Gathered(done)
      case GatherFailed(fault, at) => GatherFailed(fault, at)
    steps = steps + 1
  Stigi.Syntax.Parse.finishGather(tables, base, gathering, token)
