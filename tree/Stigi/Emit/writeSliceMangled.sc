// prompt: write a dotted source slice mangled to a C identifier with optional Stigi prefix.
def Stigi.Emit.writeSliceMangled {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), sliceStart: U64, sliceEnd: U64, prefixStigi: Bool, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(sliceStart, sliceEnd), lessOrEqual(sliceEnd, srcCap), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = if prefixStigi then Stigi.Emit.writeLiteral(6, "Stigi_", outCap, output, writeOffset) else writeOffset
  var charIdx: U64 = sliceStart
  while charIdx < sliceEnd do
    @<lessOrEqual(sliceStart, charIdx), lessOrEqual(charIdx, sliceEnd), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(sliceEnd - charIdx)>
    val rawByte: U8 = srcBytes(charIdx)
    val mappedByte: U8 = if rawByte == 46 then 95 else rawByte
    pos = Stigi.Emit.writeByte(outCap, output, pos, mappedByte)
    charIdx = charIdx + 1
  pos
