// prompt: add the edge from the unit that a rule or a given extends to this unit, when the reference node names a unit of the image.
def Stigi.Names.extendEdge {Stigi.Mut} (edges: Stigi.Names.Edges, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, at: U64) <below(from, Stigi.Limit.maxUnits), below(at, Stigi.Limit.maxUnitNodes)>: Stigi.Names.Edges =
  Stigi.Names.referenceNameAt(tables, at) match
    case Nameless => edges
    case NameFound(text) =>
      val Text(first, end) = text
      Stigi.Names.resolveText(image, members, tables, from, first, end, false) match
        case ToUnit(entry) => if entry < Stigi.Limit.maxUnits then Stigi.Names.addEdge(edges, entry, from) else edges
        case ToCase(_, _) | ToBuiltin | Unresolved | AmbiguousCase => edges
