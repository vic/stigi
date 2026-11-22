// prompt: compare a proved slice of source bytes against a target byte array.
pub def Stigi.Parse.sliceEquals(sourceLength: U64, source: Array[U8](sourceLength), startOffset: U64, endOffset: U64, targetLength: U64, target: Array[U8](targetLength)) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, sourceLength), below(sourceLength, 0x100000000), below(targetLength, 0x100000000)>: Bool =
  val sliceLength: U64 = endOffset - startOffset
  if sliceLength != targetLength then
    false
  else
    var offset: U64 = 0
    var matched: Bool = true
    while offset < targetLength && matched do
      @<lessOrEqual(offset, targetLength), decreases(targetLength - offset)>
      val sourcePos: U64 = startOffset + offset
      if sourcePos < sourceLength then
        if source(sourcePos) != target(offset) then
          matched = false
          offset = offset + 1
        else
          offset = offset + 1
      else
        matched = false
        offset = offset + 1
    matched
