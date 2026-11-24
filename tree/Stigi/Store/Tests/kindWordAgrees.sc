// prompt: whether the size def of a unit kind word equals the bytes the kind word writer puts at a position.
def Stigi.Store.Tests.kindWordAgrees {Stigi.Mut} (report: Array[U8](65536), at: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(at, 65536)>: Bool =
  Stigi.Store.kindWordSize(kind) + at == Stigi.Store.putKindWord(65536, report, at, kind)
