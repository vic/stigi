// prompt: check whether a token span equals a given byte literal.
pub def Stigi.Emit.tokenEquals(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokIndex: U64, textLen: U64, text: Array[U8](textLen)) <below(tokIndex, tokCap), below(srcCap, 0x100000000), below(textLen, 0x100000000)>: Bool =
  val startPos: U64 = tokStart(tokIndex)
  val endPos: U64 = tokEnd(tokIndex)
  if startPos <= endPos && endPos <= srcCap then
    Stigi.Parse.sliceEquals(srcCap, srcBytes, startPos, endPos, textLen, text)
  else
    false
