// prompt: one unit of the names image: the byte run of its name in the name pool, its kind and its pub flag.
enum Stigi.Names.Entry:
  case Entry(first: U64, end: U64, kind: Stigi.Names.UnitKind, isPublic: Bool) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>
