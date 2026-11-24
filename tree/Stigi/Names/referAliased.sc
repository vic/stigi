// prompt: record one reference whose text was rewritten by a unit alias: the search of the alias target joined with the tail of the reference gives an edge to the unit it names, else an unresolved reference.
def Stigi.Names.referAliased {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, tables: Stigi.Syntax.Tables, from: U64, target: Stigi.Syntax.Text, tail: Stigi.Syntax.Text) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  Stigi.Names.searchAliased(image, tables, from, target, tail) match
    case UnitFound(entry) => Expanded(Stigi.Names.walkEdge(walker, from, entry), ReferenceCounted)
    case UnitAbsent => Expanded(walker, UnresolvedReferenceCounted)
