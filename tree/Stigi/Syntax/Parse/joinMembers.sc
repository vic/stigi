// prompt: join a trait or given head and its parsed defs into a trait or a given.
def Stigi.Syntax.Parse.joinMembers {Stigi.Mut} (tables: Stigi.Syntax.Tables, isPublic: Bool, kind: Stigi.Syntax.Parse.CompoundKind, head: Stigi.Syntax.Parse.CompoundHead, members: Stigi.Syntax.Parse.Listed) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  (head, members) match
    case (CompoundHead(name, typeParams, capParams, _, _, target, _), Listed(defs, after)) =>
      Stigi.Syntax.Parse.claimOver(after, target, target, target) match
        case Unclaimed(fault) => Failed(fault, target)
        case Claimed(at, implemented, _, _) => if kind is IsGiven then Stigi.Syntax.Parse.place(tables, at, GivenDecl(at, isPublic, name, typeParams, capParams, implemented, defs), after) else Stigi.Syntax.Parse.place(tables, at, TraitDecl(at, isPublic, name, typeParams, defs), after)
    case (CompoundHeadFailed(fault, at), _) => Failed(fault, at)
    case (CompoundHead(_, _, _, _, _, _, _), ListFailed(fault, at)) => Failed(fault, at)
