// prompt: join a capability head, its states and its parsed members into a capability.
def Stigi.Syntax.Parse.joinCapability {Stigi.Mut} (tables: Stigi.Syntax.Tables, isPublic: Bool, head: Stigi.Syntax.Parse.CompoundHead, states: Stigi.Syntax.Parse.States, members: Stigi.Syntax.Parse.Listed) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  (head, states, members) match
    case (CompoundHead(name, typeParams, _, _, _, _, _), States(stateNames, startState, stops, _), Listed(defs, after)) =>
      Stigi.Syntax.Parse.claimOver(after, startState, startState, startState) match
        case Unclaimed(fault) => Failed(fault, startState)
        case Claimed(at, started, _, _) => Stigi.Syntax.Parse.place(tables, at, CapabilityDecl(at, isPublic, name, typeParams, stateNames, started, stops, defs), after)
    case (CompoundHeadFailed(fault, at), _, _) => Failed(fault, at)
    case (CompoundHead(_, _, _, _, _, _, _), StatesFailed(fault, at), _) => Failed(fault, at)
    case (CompoundHead(_, _, _, _, _, _, _), States(_, _, _, _), ListFailed(fault, at)) => Failed(fault, at)
