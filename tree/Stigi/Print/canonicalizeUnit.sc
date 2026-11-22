// prompt: copy a unit into canonical form in one forward pass by trimming trailing spaces before newlines.
pub def Stigi.Print.canonicalizeUnit {Stigi.Mut} (sourceCap: U64, source: Array[U8](sourceCap), sourceLength: U64, outputCap: U64, output: Array[U8](outputCap)) <lessOrEqual(sourceLength, sourceCap), below(sourceCap, 0x100000000), below(outputCap, 0x100000000)>: U64 <lessOrEqual(result, outputCap)> =
  var cursor: U64 = 0
  var outCursor: U64 = 0
  var stepIndex: U64 = 0
  while cursor < sourceLength && outCursor < outputCap && stepIndex < sourceLength do
    @<lessOrEqual(cursor, sourceLength), lessOrEqual(outCursor, outputCap), lessOrEqual(stepIndex, sourceLength), decreases(sourceLength - stepIndex)>
    val lineStart: U64 = cursor
    var trimEnd: U64 = cursor
    while cursor < sourceLength && source(cursor) != 10 do
      @<lessOrEqual(lineStart, trimEnd), lessOrEqual(trimEnd, cursor), lessOrEqual(cursor, sourceLength), decreases(sourceLength - cursor)>
      val byte: U8 = source(cursor)
      cursor = cursor + 1
      trimEnd = if byte != 32 then cursor else trimEnd
    outCursor = Stigi.Print.writeSlice(sourceCap, source, lineStart, trimEnd, outputCap, output, outCursor)
    if cursor < sourceLength && outCursor < outputCap then
      output(outCursor) := 10
      outCursor = outCursor + 1
      cursor = cursor + 1
    else
      cursor = sourceLength
    stepIndex = stepIndex + 1
  outCursor
