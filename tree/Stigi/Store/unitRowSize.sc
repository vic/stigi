// prompt: the bytes putUnitRow writes: the name, a tab, the kind word and a newline.
def Stigi.Store.unitRowSize(first: U64, end: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: U64 =
  end - first + Stigi.Store.kindWordSize(kind) + 2
