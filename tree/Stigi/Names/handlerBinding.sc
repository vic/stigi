// prompt: the capability a handle statement binds: the capability unit that the invoked handler stands for, searched from the handler namespace; a handler whose capability name is not found among the units keeps no capability, so an operation call on its binder stays unresolved, which rejects more than the seed does for builtin capabilities; an invoked name rewritten by a unit alias is out of scope; an entry past the unit count is a malformed read.
def Stigi.Names.handlerBinding(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64) <below(from, Stigi.Limit.maxUnits), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.CapabilityRead =
  Stigi.Names.searchName(image, tables, from, first, end) match
    case UnitAbsent => CapabilityFound(WithoutCapability)
    case UnitFound(invokedEntry) =>
      image match
        case Image(_, entries, count, _) =>
          if invokedEntry < count then
            entries(invokedEntry) match
              case Entry(_, _, kind, _) =>
                kind match
                  case HandlerKind =>
                    Stigi.Names.forCapability(members, invokedEntry, 0, Stigi.Names.maxMembers) match
                      case MissingFor => CapabilityFound(WithoutCapability)
                      case ForText(forFirst, forEnd) =>
                        Stigi.Names.searchPoolName(image, members, invokedEntry, forFirst, forEnd) match
                          case UnitAbsent => CapabilityFound(WithoutCapability)
                          case UnitFound(boundEntry) => Stigi.Names.capabilityUnit(image, boundEntry)
                  case DefKind | ValKind | EnumKind | TypeKind | CapabilityKind | TraitKind | GivenKind | ExternKind | RelationKind | RuleKind | TestKind | NegTestKind => CapabilityFound(WithoutCapability)
          else CapabilityMalformed
