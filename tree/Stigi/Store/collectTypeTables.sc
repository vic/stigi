// prompt: populate the sorted typeRange and typeWidth fact rows for every L0 integer primitive from the given starting offsets.
def Stigi.Store.collectTypeTables {region: Stigi.Region, Stigi.Mut} (rangeBuf: Array[U8](4096), widthBuf: Array[U8](4096), rangeStart: U64, widthStart: U64) <lessOrEqual(rangeStart, 4096), lessOrEqual(widthStart, 4096)>: Stigi.Store.TypeRowCursors =
  val TypeCursors(rangeWord, widthWord) = Stigi.Store.appendPrimitiveFacts{region}(rangeBuf, widthBuf, rangeStart, widthStart, SignedWord)
  val TypeCursors(rangeLong, widthLong) = Stigi.Store.appendPrimitiveFacts{region}(rangeBuf, widthBuf, rangeWord, widthWord, SignedLong)
  val TypeCursors(rangeUword, widthUword) = Stigi.Store.appendPrimitiveFacts{region}(rangeBuf, widthBuf, rangeLong, widthLong, UnsignedWord)
  val TypeCursors(rangeUlong, widthUlong) = Stigi.Store.appendPrimitiveFacts{region}(rangeBuf, widthBuf, rangeUword, widthUword, UnsignedLong)
  Stigi.Store.appendPrimitiveFacts{region}(rangeBuf, widthBuf, rangeUlong, widthUlong, UnsignedByte)
