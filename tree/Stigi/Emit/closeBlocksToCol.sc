// prompt: close open block stack frames down to the target indentation column emitting C braces and frees.
pub def Stigi.Emit.closeBlocksToCol {region: Stigi.Region, Stigi.Mut} (blkKinds: Array[U8](32), blkCols: Array[U64](32), blkSavedLocals: Array[U64](32), blkTempNums: Array[U64](32), blkPrev: Array[U64](32), blkTopBox: Array[U64](1), localCountBox: Array[U64](1), targetBox: Array[U64](4), nextCol: U64, keepEqualCol: Bool, closeAll: Bool, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = writeOffset
  var walkStep: U64 = 0
  var active: Bool = true
  while walkStep < 32 && active do
    @<lessOrEqual(walkStep, 32), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(32 - walkStep)>
    val curTop: U64 = blkTopBox(0)
    if curTop < 32 then
      val curKind: U8 = blkKinds(curTop)
      val curCol: U64 = blkCols(curTop)
      val shouldClose: Bool = if closeAll then curKind != 1 else if curKind == 1 then false else if keepEqualCol then curCol > nextCol else curCol >= nextCol
      if shouldClose then
        if curKind == 7 then
          pos = Stigi.Emit.writeLiteral(10, "free(_tptr", outCap, output, pos)
          pos = Stigi.Emit.writeUnsignedDecimal{region}(blkTempNums(curTop), outCap, output, pos)
          pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
          pos = Stigi.Emit.writeLine(1, "}", outCap, output, pos)
        else if curKind == 9 then
          pos = Stigi.Emit.writeLine(8, "break; }", outCap, output, pos)
        else if curKind == 8 then
          pos = Stigi.Emit.writeLine(18, "default: break; }}", outCap, output, pos)
        else
          pos = Stigi.Emit.writeLine(1, "}", outCap, output, pos)
        localCountBox(0) := blkSavedLocals(curTop)
        if targetBox(0) == 1 && targetBox(3) == curTop then
          targetBox(0) := 0
        else
          ()
        blkTopBox(0) := blkPrev(curTop)
      else
        active = false
    else
      active = false
    walkStep = walkStep + 1
  pos
