// prompt: the outcome of a search of the image for a unit: the entry of the unit it found, below the unit limit, or none.
enum Stigi.Names.UnitLookup:
  case UnitFound(entry: U64) <below(entry, Stigi.Limit.maxUnits)>
  case UnitAbsent
