// prompt: copy a proved source slice into an output byte array and return the advanced write offset.
pub def Stigi.Print.writeSlice {Stigi.Mut} (sourceLength: U64, source: Array[U8](sourceLength), startOffset: U64, endOffset: U64, outputCap: U64, output: Array[U8](outputCap), writeOffset: U64) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, sourceLength), lessOrEqual(writeOffset, outputCap)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outputCap)> =
  var cursor: U64 = startOffset
  var outCursor: U64 = writeOffset
  while cursor < endOffset && outCursor < outputCap do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, endOffset), lessOrEqual(writeOffset, outCursor), lessOrEqual(outCursor, outputCap), decreases(endOffset - cursor)>
    output(outCursor) := source(cursor)
    cursor = cursor + 1
    outCursor = outCursor + 1
  outCursor
