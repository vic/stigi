// prompt: one member: the unit entry that owns it, its kind, and the byte run of its text in the member pool.
enum Stigi.Names.Member:
  case Member(owner: U64, kind: Stigi.Names.MemberKind, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>
