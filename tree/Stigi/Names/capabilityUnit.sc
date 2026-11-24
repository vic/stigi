// prompt: the capability binding of one unit entry: the unit itself when it is a capability or a trait, else none.
def Stigi.Names.capabilityUnit(image: Stigi.Names.Image, entry: U64) <below(entry, Stigi.Limit.maxUnits)>: Stigi.Names.CapabilityBinding =
  image match
    case Image(_, entries, count, _) =>
      if entry < count then
        entries(entry) match
          case Entry(_, _, kind, _) =>
            kind match
              case CapabilityKind | TraitKind => CapabilityUnit(entry)
              case DefKind | ValKind | EnumKind | TypeKind | HandlerKind | GivenKind | ExternKind | RelationKind | RuleKind | TestKind | NegTestKind => WithoutCapability
      else WithoutCapability
