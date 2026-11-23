// prompt: emit the C type for an L0 type expression starting at a token index.
pub def Stigi.Emit.emitTypeName {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, typeTok: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(typeTok, tokCount), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, typeTok, 5, "Array") then
    Stigi.Emit.emitArrayElemType(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, typeTok, outCap, output, writeOffset)
  else
    val startPos: U64 = tokStart(typeTok)
    val endPos: U64 = tokEnd(typeTok)
    if startPos <= endPos && endPos <= srcCap then
      Stigi.Emit.emitTypeSlice(srcCap, srcBytes, startPos, endPos, outCap, output, writeOffset)
    else
      writeOffset
