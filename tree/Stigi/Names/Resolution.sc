// prompt: what one reference names: a unit, an enum case of a unit, a builtin, nothing, or more than one enum case.
enum Stigi.Names.Resolution:
  case ToUnit(entry: U64)
  case ToCase(owner: U64, member: U64)
  case ToBuiltin
  case Unresolved
  case AmbiguousCase
