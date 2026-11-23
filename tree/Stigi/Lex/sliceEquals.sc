// prompt: compare a proved source byte slice against an exact literal byte array.
pub def Stigi.Lex.sliceEquals(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64, litLen: U64, lit: Array[U8](litLen)) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap), below(litLen, Stigi.Limit.maxSourceBytes)>: Bool =
  if endPos - startPos != litLen then false else
    var idx: U64 = 0
    var matched: Bool = true
    while idx < litLen && matched do
      @<lessOrEqual(idx, litLen)>
      val srcPos: U64 = startPos + idx
      if srcPos < srcCap then
        if src(srcPos) != lit(idx) then matched = false else ()
      else
        matched = false
      idx = idx + 1
    matched
