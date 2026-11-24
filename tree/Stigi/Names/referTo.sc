// prompt: record one reference of a unit walk: a unit alias rewrites the name before the search, a local capability operation adds an edge to its capability, any other local counts nothing, and a name that is no local adds an edge to the unit or enum owner it names; the result reports whether it counts as a reference or as an unresolved reference.
def Stigi.Names.referTo {Stigi.Mut} (walker: Stigi.Names.Walker, scope: Stigi.Names.NameScope, tables: Stigi.Syntax.Tables, from: U64, text: Stigi.Syntax.Text, allowCase: Bool) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  val Text(first, end) = text
  if end <= first then Expanded(walker, NothingCounted) else
    val Walker(_, _, locals, localCount, _, _, _, _, _) = walker
    Stigi.Names.aliasFor(tables, locals, localCount, first, end, 0, localCount) match
      case AliasHit(target, tail) =>
        scope match
          case NameScope(image, _) => Stigi.Names.referAliased(walker, image, tables, from, target, tail)
      case Unaliased =>
        Stigi.Names.localBinding(tables, locals, localCount, first, end, localCount) match
          case LocalBound(whole) =>
            whole match
              case CapabilityUnit(_) | BuiltinCapability | WithoutCapability => Expanded(walker, NothingCounted)
          case NotLocal =>
            val Tables(source, _, _, _, _, _) = tables
            val dotAt: U64 = Stigi.Names.lastDot(source, first, end, end - first + 1)
            if first < dotAt && dotAt < end && dotAt + 1 < end then
              Stigi.Names.localBinding(tables, locals, localCount, first, dotAt, localCount) match
                case NotLocal => Stigi.Names.referResolved(walker, scope, tables, from, text, allowCase)
                case LocalBound(binding) =>
                  binding match
                    case CapabilityUnit(owner) =>
                      scope match
                        case NameScope(_, members) => Stigi.Names.referOperation(walker, members, tables, from, owner, Text(dotAt + 1, end))
                    case BuiltinCapability | WithoutCapability => Stigi.Names.referResolved(walker, scope, tables, from, text, allowCase)
            else Stigi.Names.referResolved(walker, scope, tables, from, text, allowCase)
