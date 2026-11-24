// prompt: record one reference of a unit walk by the namepath search alone: an edge to the unit or enum it names, else the capability operation its prefix names.
def Stigi.Names.referResolved {Stigi.Mut} (walker: Stigi.Names.Walker, scope: Stigi.Names.NameScope, tables: Stigi.Syntax.Tables, from: U64, text: Stigi.Syntax.Text, allowCase: Bool) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  Stigi.Names.resolveText(scope, tables, from, text, allowCase) match
    case ToUnit(entry) => Expanded(Stigi.Names.walkEdge(walker, from, entry), ReferenceCounted)
    case ToCase(owner, _) => Expanded(Stigi.Names.walkEdge(walker, from, owner), ReferenceCounted)
    case ToBuiltin => Expanded(walker, ReferenceCounted)
    case Unresolved | AmbiguousCase =>
      scope match
        case NameScope(image, members) => Stigi.Names.referViaPrefix(walker, image, members, tables, from, text)
