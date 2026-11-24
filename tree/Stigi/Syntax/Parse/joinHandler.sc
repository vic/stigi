// prompt: join a handler head, its var lines and its parsed defs into a handler.
def Stigi.Syntax.Parse.joinHandler {Stigi.Mut} (tables: Stigi.Syntax.Tables, isPublic: Bool, head: Stigi.Syntax.Parse.CompoundHead, vars: Stigi.Syntax.Span, members: Stigi.Syntax.Parse.Listed) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  (head, members) match
    case (CompoundHead(name, typeParams, capParams, valueGroup, valueParams, target, _), Listed(defs, after)) =>
      Stigi.Syntax.Parse.claimOver(after, target, target, target) match
        case Unclaimed(fault) => Failed(fault, target)
        case Claimed(at, handled, _, _) => Stigi.Syntax.Parse.place(tables, at, HandlerDecl(at, isPublic, name, typeParams, capParams, valueGroup, valueParams, handled, vars, defs), after)
    case (CompoundHeadFailed(fault, at), _) => Failed(fault, at)
    case (CompoundHead(_, _, _, _, _, _, _), ListFailed(fault, at)) => Failed(fault, at)
