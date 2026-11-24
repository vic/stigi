// prompt: map the fault and the unresolved reference count a unit walk ended with to its outcome: a sound walk with every reference resolved keeps its edges, a sound walk with an unresolved reference fails with that reason and every other fault fails the walk with its own reason.
def Stigi.Names.walkVerdict(fault: Stigi.Names.WalkFault, unresolved: U64, edges: Stigi.Names.Edges): Stigi.Names.WalkOutcome =
  fault match
    case WalkSound => if unresolved < 1 then Walked(edges) else WalkFailed(UnresolvedNames)
    case StackFull => WalkFailed(WalkFaulted(ReasonStack))
    case ScopeFull => WalkFailed(WalkFaulted(ReasonScope))
    case TableMalformed => WalkFailed(WalkFaulted(ReasonTable))
    case StepsExhausted => WalkFailed(WalkFaulted(ReasonSteps))
    case EdgesExhausted => WalkFailed(WalkFaulted(ReasonEdges))
