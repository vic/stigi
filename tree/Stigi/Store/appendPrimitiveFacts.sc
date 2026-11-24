// prompt: append the typeRange and typeWidth rows for one primitive type when it is an integer, giving the updated cursors.
def Stigi.Store.appendPrimitiveFacts {region: Stigi.Region, Stigi.Mut} (rangeBuf: Array[U8](4096), widthBuf: Array[U8](4096), rangeStart: U64, widthStart: U64, primitive: Stigi.Core.Primitive) <lessOrEqual(rangeStart, 4096), lessOrEqual(widthStart, 4096)>: Stigi.Store.TypeRowCursors =
  Stigi.Types.integerInfoOf(primitive) match
    case IntSpec(low, high, width) =>
      val nextRange: U64 = Stigi.Store.putTypeRangeRow{region}(4096, rangeBuf, rangeStart, primitive, low, high)
      val nextWidth: U64 = Stigi.Store.putTypeWidthRow{region}(4096, widthBuf, widthStart, primitive, width)
      TypeCursors(nextRange, nextWidth)
    case NonInteger => TypeCursors(rangeStart, widthStart)
