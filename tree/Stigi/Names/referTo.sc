// prompt: record one reference of a unit walk: a local capability operation adds an edge to its capability, a name that is no local adds an edge to the unit or enum owner it names, and the result reports whether it counts as a reference or as an unresolved reference.
def Stigi.Names.referTo {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, text: Stigi.Syntax.Text, allowCase: Bool) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  val Text(first, end) = text
  if end <= first || Stigi.Names.isLocal(tables, walker, first, end) then Expanded(walker, NothingCounted) else
    val Walker(_, _, locals, localCount, _, _, _, _, _) = walker
    val Tables(source, _, _, _, _, _) = tables
    val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
    if first < dotAt && dotAt < end && dotAt + 1 < end then
      Stigi.Names.localBinding(tables, locals, localCount, first, dotAt, localCount) match
        case NotLocal => Stigi.Names.referResolved(walker, image, members, tables, from, first, end, allowCase)
        case LocalBound(binding) =>
          binding match
            case CapabilityUnit(owner) => Stigi.Names.referOperation(walker, members, tables, from, owner, dotAt + 1, end)
            case BuiltinCapability | WithoutCapability => Stigi.Names.referResolved(walker, image, members, tables, from, first, end, allowCase)
    else Stigi.Names.referResolved(walker, image, members, tables, from, first, end, allowCase)
