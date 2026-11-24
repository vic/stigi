// prompt: the capability binding of one unit entry: the unit itself when it is a capability or a trait, else none; an entry past the unit count is a malformed read.
def Stigi.Names.capabilityUnit(image: Stigi.Names.Image, entry: U64) <below(entry, Stigi.Limit.maxUnits)>: Stigi.Names.CapabilityRead =
  image match
    case Image(_, entries, count, _) =>
      if entry < count then
        entries(entry) match
          case Entry(_, _, kind, _) =>
            kind match
              case CapabilityKind | TraitKind => CapabilityFound(CapabilityUnit(entry))
              case DefKind | ValKind | EnumKind | TypeKind | HandlerKind | GivenKind | ExternKind | RelationKind | RuleKind | TestKind | NegTestKind => CapabilityFound(WithoutCapability)
      else CapabilityMalformed
