// prompt: compare a proved source byte slice against an exact literal byte array.
pub def Stigi.Lex.sliceEquals(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64, litLen: U64, lit: Array[U8](litLen)) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap), below(litLen, Stigi.Limit.maxSourceBytes)>: Bool =
  endPos - startPos == litLen && Stigi.Lex.sliceTail(srcCap, src, startPos, litLen, lit, litLen)
