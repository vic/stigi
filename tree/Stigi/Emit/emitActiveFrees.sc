// prompt: emit free calls for all active tryFill allocations currently live on the block stack.
def Stigi.Emit.emitActiveFrees {region: Stigi.Region, Stigi.Mut} (blkKinds: Array[U8](96), blkTempNums: Array[U64](96), blkPrev: Array[U64](96), blkTop: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = writeOffset
  var curIdx: U64 = blkTop
  var walkStep: U64 = 0
  while walkStep < 96 && curIdx < 96 do
    @<lessOrEqual(walkStep, 96), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(96 - walkStep)>
    if blkKinds(curIdx) == 7 then
      pos = Stigi.Emit.writeLiteral(10, "free(_tptr", outCap, output, pos)
      pos = Stigi.Emit.writeUnsignedDecimal{region}(blkTempNums(curIdx), outCap, output, pos)
      pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
    else
      ()
    curIdx = blkPrev(curIdx)
    walkStep = walkStep + 1
  pos
