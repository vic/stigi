// prompt: build a verified type layout record for any primitive kind within a maximum alignment.
pub def Stigi.Types.primLayout(kind: Stigi.Types.PrimKind, maxAlign: U64) <lessOrEqual(8, maxAlign)>: Stigi.Types.Layout =
  val byteSize: U64 = Stigi.Types.primSize(kind)
  val alignBytes: U64 = Stigi.Types.primAlign(kind)
  Layout(byteSize, alignBytes@<lessOrEqual(alignBytes, maxAlign)>)
