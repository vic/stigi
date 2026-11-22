// prompt: verify whether a 64-byte slice contains strictly valid lowercase hexadecimal digest characters.
def Stigi.Store.isHexDigestSlice(cap: U64, text: Array[U8](cap), startOffset: U64, endOffset: U64) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, cap), below(cap, 0x100000000)>: Bool =
  if endOffset - startOffset != 64 then
    false
  else
    var cursor: U64 = startOffset
    var valid: Bool = true
    while cursor < endOffset && valid do
      @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, endOffset), decreases(endOffset - cursor)>
      if Stigi.Store.hexDigitValue(text(cursor)) >= 16 then
        valid = false
      else
        ()
      cursor = cursor + 1
    valid
