// prompt: decode an unsigned decimal integer token slice from source bytes without overflow.
def Stigi.Eval.parseUintSlice(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap)>: Stigi.Eval.Value =
  var pos: U64 = startPos
  var acc: U64 = 0
  var valid: Bool = startPos < endPos
  while pos < endPos && valid do
    @<lessOrEqual(pos, endPos)>
    val byte: U8 = src(pos)
    if byte >= 48 && byte <= 57 then
      val digit: U64 = U64.ofU8(byte) - 48
      if acc < 1000000000000000 then
        val scaled: U64 = acc * 10
        if scaled < 10000000000000000 && digit < 10 then
          acc = scaled + digit
        else
          valid = false
      else
        valid = false
    else
      valid = false
    pos = pos + 1
  if valid then ValWord(acc) else ValOverflow
