// prompt: the kind and pub flag of a parsed unit, or no shape when its table is malformed.
enum Stigi.Names.UnitShape:
  case Shaped(kind: Stigi.Names.UnitKind, isPublic: Bool)
  case Unshaped
