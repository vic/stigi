// prompt: write the kind word of a unit kind, as the unit relation names it, at a position, giving the new end.
def Stigi.Store.putKindWord {Stigi.Mut} (reportCap: Stigi.EmitCap, report: Array[U8](reportCap), at: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(at, reportCap)>: U64 <lessOrEqual(result, reportCap)> =
  kind match
    case DefKind => Stigi.Array.copy[U8](3, "Def", 0, 3, reportCap, report, at)
    case ValKind => Stigi.Array.copy[U8](3, "Val", 0, 3, reportCap, report, at)
    case EnumKind => Stigi.Array.copy[U8](4, "Enum", 0, 4, reportCap, report, at)
    case TypeKind => Stigi.Array.copy[U8](4, "Type", 0, 4, reportCap, report, at)
    case CapabilityKind => Stigi.Array.copy[U8](10, "Capability", 0, 10, reportCap, report, at)
    case HandlerKind => Stigi.Array.copy[U8](7, "Handler", 0, 7, reportCap, report, at)
    case TraitKind => Stigi.Array.copy[U8](5, "Trait", 0, 5, reportCap, report, at)
    case GivenKind => Stigi.Array.copy[U8](5, "Given", 0, 5, reportCap, report, at)
    case ExternKind => Stigi.Array.copy[U8](6, "Extern", 0, 6, reportCap, report, at)
    case RelationKind => Stigi.Array.copy[U8](8, "Relation", 0, 8, reportCap, report, at)
    case RuleKind => Stigi.Array.copy[U8](4, "Rule", 0, 4, reportCap, report, at)
    case TestKind => Stigi.Array.copy[U8](4, "Test", 0, 4, reportCap, report, at)
    case NegTestKind => Stigi.Array.copy[U8](7, "NegTest", 0, 7, reportCap, report, at)
