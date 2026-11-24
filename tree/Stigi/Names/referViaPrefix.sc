// prompt: record a reference whose text did not resolve whole: when its prefix names a capability or trait with that operation it is an edge to that unit.
def Stigi.Names.referViaPrefix {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, text: Stigi.Syntax.Text) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  val Text(first, end) = text
  val Tables(source, _, _, _, _, _) = tables
  val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
  if first < dotAt && dotAt < end && dotAt + 1 < end then
    Stigi.Names.resolveText(NameScope(image, members), tables, from, Text(first, dotAt), false) match
      case ToUnit(owner) =>
        Stigi.Names.capabilityUnit(image, owner) match
          case CapabilityFound(CapabilityUnit(unit)) => Stigi.Names.referOperation(walker, members, tables, from, unit, Text(dotAt + 1, end))
          case CapabilityFound(BuiltinCapability) | CapabilityFound(WithoutCapability) => Expanded(walker, UnresolvedReferenceCounted)
          case CapabilityMalformed => Expanded(Stigi.Names.failWith(walker, TableMalformed), NothingCounted)
      case ToCase(_, _) | ToBuiltin | Unresolved | AmbiguousCase => Expanded(walker, UnresolvedReferenceCounted)
  else Expanded(walker, UnresolvedReferenceCounted)
