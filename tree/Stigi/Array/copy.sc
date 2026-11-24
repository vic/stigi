// prompt: copy elements from src[srcStart..srcEnd) into dst starting at dstStart until either range ends, returning the new destination offset.
pub extern "c" def Stigi.Array.copy [A: Type] {Stigi.Mut} (srcCap: U64, src: Array[A](srcCap), srcStart: U64, srcEnd: U64, dstCap: U64, dst: Array[A](dstCap), dstStart: U64) <lessOrEqual(srcStart, srcEnd), lessOrEqual(srcEnd, srcCap), lessOrEqual(dstStart, dstCap)>: U64 <lessOrEqual(dstStart, result), lessOrEqual(result, dstCap)> =
  var srcIdx: U64 = srcStart
  var dstIdx: U64 = dstStart
  while srcIdx < srcEnd && dstIdx < dstCap do
    @<lessOrEqual(srcIdx, srcEnd), lessOrEqual(dstStart, dstIdx), lessOrEqual(dstIdx, dstCap)>
    dst(dstIdx) := src(srcIdx)
    srcIdx = srcIdx + 1
    dstIdx = dstIdx + 1
  dstIdx
