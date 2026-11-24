// prompt: one unit to index: the bytes of its name and their count, and the bytes of its source and their count.
enum Stigi.Names.UnitSource:
  case UnitSource(name: Array[U8](Stigi.Limit.maxUnitBytes), nameSize: U64, source: Array[U8](Stigi.Limit.maxUnitBytes), size: U64) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes), lessOrEqual(size, Stigi.Limit.maxUnitBytes)>
