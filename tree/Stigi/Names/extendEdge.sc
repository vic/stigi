// prompt: add the edge from the unit that a rule or a given extends to this unit, when the reference node names a unit of the image: an edge past the limit fails the walk.
def Stigi.Names.extendEdge {Stigi.Mut} (edges: Stigi.Names.Edges, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, at: U64) <below(from, Stigi.Limit.maxUnits), below(at, Stigi.Limit.maxUnitNodes)>: Stigi.Names.WalkOutcome =
  Stigi.Names.referenceNameAt(tables, at) match
    case Nameless => WalkFailed(WalkFaulted(ReasonTable))
    case NameFound(text) =>
      Stigi.Names.resolveText(NameScope(image, members), tables, from, text, false) match
        case ToUnit(entry) =>
          Stigi.Names.addEdge(edges, entry, from) match
            case EdgeKept(next) => Walked(next)
            case EdgesFull => WalkFailed(WalkFaulted(ReasonEdges))
        case ToCase(_, _) | ToBuiltin => Walked(edges)
        case Unresolved | AmbiguousCase => WalkFailed(UnresolvedNames)
