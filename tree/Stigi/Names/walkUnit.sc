// prompt: walk one parsed unit from its entry: visit its root, then add the edges its rule head or given trait owes, or fail when the walk stopped early or left a reference unresolved.
def Stigi.Names.walkUnit {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, root: Stigi.Syntax.NodeAt) <below(from, Stigi.Limit.maxUnits), below(root, Stigi.Limit.maxUnitNodes)>: Stigi.Names.WalkOutcome =
  val Walker(_, _, _, _, walked, _, unresolved, _, fault) = Stigi.Names.walkFrom(Stigi.Names.pushTask(walker, Visit(root)), image, members, tables, from)
  Stigi.Names.walkVerdict(fault, unresolved, walked) match
    case Walked(sound) => Stigi.Names.unitEdges(sound, image, members, tables, from, root)
    case WalkFailed(why) => WalkFailed(why)
