// prompt: what one reference names: a unit, an enum case of a unit, a builtin, nothing, or more than one enum case.
enum Stigi.Names.Resolution:
  case ToUnit(entry: U64) <below(entry, Stigi.Limit.maxUnits)>
  case ToCase(owner: U64, member: U64) <below(owner, Stigi.Limit.maxUnits)>
  case ToBuiltin
  case Unresolved
  case AmbiguousCase
