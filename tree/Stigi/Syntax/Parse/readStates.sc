// prompt: read the state, start and stop lines of a capability after a line break, or empty states when the first line is no state line.
def Stigi.Syntax.Parse.readStates {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.States =
  val Progress(token, nodes, lists, pending) = start
  val first: U64 = Stigi.Syntax.Parse.lineStart(tables, token, Stigi.Limit.maxUnitBytes)
  if !(Stigi.Syntax.Parse.kindAt(tables, first) is TokState) then
    Stigi.Syntax.Parse.absent(tables, start) match
      case Failed(fault, at) => StatesFailed(fault, at)
      case Built(missing, after) =>
        Stigi.Syntax.Parse.emptyList(after) match
          case ListFailed(fault, at) => StatesFailed(fault, at)
          case Listed(none, _) => States(none, missing, none, after)
  else
    Stigi.Syntax.Parse.readNames(tables, Progress(Stigi.Syntax.Parse.nextToken(first), nodes, lists, pending), LineEnd) match
      case ListFailed(fault, at) => StatesFailed(fault, at)
      case Listed(states, afterStates) =>
        val Progress(stateEnd, stateNodes, stateLists, statePending) = afterStates
        val startLine: U64 = Stigi.Syntax.Parse.lineStart(tables, stateEnd, Stigi.Limit.maxUnitBytes)
        if !(Stigi.Syntax.Parse.kindAt(tables, startLine) is TokStart) then StatesFailed(UnexpectedToken, startLine) else
          Stigi.Syntax.Parse.parseName(tables, Progress(Stigi.Syntax.Parse.nextToken(startLine), stateNodes, stateLists, statePending)) match
            case Failed(fault, at) => StatesFailed(fault, at)
            case Built(startState, afterStart) =>
              val Progress(startEnd, startNodes, startLists, startPending) = afterStart
              val stopLine: U64 = Stigi.Syntax.Parse.lineStart(tables, startEnd, Stigi.Limit.maxUnitBytes)
              if !(Stigi.Syntax.Parse.kindAt(tables, stopLine) is TokStop) then StatesFailed(UnexpectedToken, stopLine) else
                Stigi.Syntax.Parse.readNames(tables, Progress(Stigi.Syntax.Parse.nextToken(stopLine), startNodes, startLists, startPending), LineEnd) match
                  case ListFailed(fault, at) => StatesFailed(fault, at)
                  case Listed(stops, afterStops) => States(states, startState, stops, afterStops)
