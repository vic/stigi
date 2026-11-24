// prompt: the byte count of the kind word of a unit kind, as the unit relation names it.
def Stigi.Store.kindWordSize(kind: Stigi.Names.UnitKind): U64 <lessOrEqual(result, 10)> =
  kind match
    case DefKind => 3
    case ValKind => 3
    case EnumKind => 4
    case TypeKind => 4
    case CapabilityKind => 10
    case HandlerKind => 7
    case TraitKind => 5
    case GivenKind => 5
    case ExternKind => 6
    case RelationKind => 8
    case RuleKind => 4
    case TestKind => 4
    case NegTestKind => 7
