// prompt: push one indentation block frame onto the 96-slot explicit lowering stack.
def Stigi.Emit.pushBlockFrame {Stigi.Mut} (blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), blkKind: U8, blkCol: U64, blkFlag: U8, savedLocals: U64, tempNum: U64, auxStart: U64, auxEnd: U64) <lessOrEqual(0, 1)>: Unit =
  val curTop: U64 = blkTopBox(0)
  if curTop == 96 then
    blkKinds(0) := blkKind
    blkCols(0) := blkCol
    blkFlags(0) := blkFlag
    blkSavedLocals(0) := savedLocals
    blkTempNums(0) := tempNum
    blkAuxStart(0) := auxStart
    blkAuxEnd(0) := auxEnd
    blkPrev(0) := 96
    blkTopBox(0) := 0
  else if curTop < 95 then
    val nextTop: U64 = curTop + 1
    blkKinds(nextTop) := blkKind
    blkCols(nextTop) := blkCol
    blkFlags(nextTop) := blkFlag
    blkSavedLocals(nextTop) := savedLocals
    blkTempNums(nextTop) := tempNum
    blkAuxStart(nextTop) := auxStart
    blkAuxEnd(nextTop) := auxEnd
    blkPrev(nextTop) := curTop
    blkTopBox(0) := nextTop
  else
    ()
