// prompt: compare the last remaining bytes of a source span with the same bytes of a literal.
def Stigi.Lex.sliceTail(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, litLen: U64, lit: Array[U8](litLen), remaining: U64) <lessOrEqual(startPos, srcCap), below(litLen, Stigi.Limit.maxSourceBytes), lessOrEqual(remaining, litLen), decreases(remaining)>: Bool =
  if remaining < 1 then true else
    val idx: U64 = remaining - 1
    val srcPos: U64 = startPos + idx
    if srcPos < srcCap && src(srcPos) == lit(idx) then Stigi.Lex.sliceTail(srcCap, src, startPos, litLen, lit, idx) else false
