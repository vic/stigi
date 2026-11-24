// prompt: read a pattern: one pattern atom, or alternatives joined by bars.
def Stigi.Syntax.Parse.readPattern {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if depth < 1 then Failed(TooDeep, token) else
    Stigi.Syntax.Parse.readPatternAtom(tables, start, depth - 1) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(first, after) =>
        val Progress(next, nodes, lists, base) = after
        val Tables(_, _, _, _, pendingTable, _) = tables
        if !(Stigi.Syntax.Parse.kindAt(tables, next) is TokBar) then Built(first, after) else if Stigi.Limit.maxUnitNodes <= base then Failed(UnitTooLarge, next) else
          Stigi.Syntax.Parse.storePending(pendingTable, base, first)
          var state: Stigi.Syntax.Parse.Gather = Gathering(Progress(next, nodes, lists, base + 1))
          var steps: U64 = 0
          while steps < Stigi.Limit.maxUnitBytes && state is Gathering(_) do
            state = state match
              case Gathering(current) =>
                val Progress(bar, currentNodes, currentLists, currentPending) = current
                if Stigi.Limit.maxUnitBytes <= bar then GatherFailed(UnexpectedToken, bar) else Stigi.Syntax.Parse.pushAlternative(tables, Stigi.Syntax.Parse.readPatternAtom(tables, Progress(bar + 1, currentNodes, currentLists, currentPending), depth - 1))
              case Gathered(done) => Gathered(done)
              case GatherFailed(fault, at) => GatherFailed(fault, at)
            steps = steps + 1
          Stigi.Syntax.Parse.finishGather(tables, base, state, next) match
            case ListFailed(fault, at) => Failed(fault, at)
            case Listed(alternatives, closed) =>
              Stigi.Syntax.Parse.freeSlot(closed) match
                case TableFull => Failed(UnitTooLarge, next)
                case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, PatOr(at, alternatives), closed)
