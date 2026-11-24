// prompt: the outcome of reading one unit named by an index line: the size of its name copy and of its source, or that it could not be read.
enum Stigi.Names.UnitLoad:
  case UnitRead(nameSize: U64, size: U64) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes), lessOrEqual(size, Stigi.Limit.maxUnitBytes)>
  case UnitMissing
