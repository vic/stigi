// prompt: record a reference whose text did not resolve whole: when its prefix names a capability or trait with that operation it is an edge to that unit.
def Stigi.Names.referViaPrefix {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64) <below(from, Stigi.Limit.maxUnits), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Expanded =
  val Tables(source, _, _, _, _, _) = tables
  val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
  if first < dotAt && dotAt < end && dotAt + 1 < end then
    Stigi.Names.resolveText(image, members, tables, from, first, dotAt, false) match
      case ToUnit(owner) =>
        if owner < Stigi.Limit.maxUnits then
          Stigi.Names.capabilityUnit(image, owner) match
            case CapabilityUnit(unit) => Stigi.Names.referOperation(walker, members, tables, from, unit, dotAt + 1, end)
            case BuiltinCapability | WithoutCapability => Expanded(walker, UnresolvedReferenceCounted)
        else Expanded(walker, UnresolvedReferenceCounted)
      case ToCase(_, _) | ToBuiltin | Unresolved | AmbiguousCase => Expanded(walker, UnresolvedReferenceCounted)
  else Expanded(walker, UnresolvedReferenceCounted)
