// prompt: record one reference of a unit walk by the namepath search alone: an edge to the unit or enum it names, else the capability operation its prefix names.
def Stigi.Names.referResolved {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64, allowCase: Bool) <below(from, Stigi.Limit.maxUnits), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Expanded =
  val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
  Stigi.Names.resolveText(image, members, tables, from, first, end, allowCase) match
    case ToUnit(entry) => Expanded(Walker(tasks, depth, locals, localCount, Stigi.Names.addEdge(edges, from, entry), references, unresolved, steps, fault), ReferenceCounted)
    case ToCase(owner, _) => Expanded(Walker(tasks, depth, locals, localCount, Stigi.Names.addEdge(edges, from, owner), references, unresolved, steps, fault), ReferenceCounted)
    case ToBuiltin => Expanded(walker, ReferenceCounted)
    case Unresolved | AmbiguousCase => Stigi.Names.referViaPrefix(walker, image, members, tables, from, first, end)
