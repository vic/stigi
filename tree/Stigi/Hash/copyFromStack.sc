// prompt: copy an eight-word chaining value out of the Merkle stack at depthIndex.
def Stigi.Hash.copyFromStack {Stigi.Mut} (stackWords: Array[U32](512), depthIndex: U64, destChain: Array[U32](8)) <below(depthIndex, 60)>: Unit =
  val startPos: U64 = depthIndex * 8
  if startPos <= 512 then
    var stackPos: U64 = startPos
    var lane: U64 = 0
    while lane < 8 && stackPos < 512 do
      @<lessOrEqual(lane, 8), lessOrEqual(stackPos, 512), decreases(8 - lane)>
      destChain(lane) := stackWords(stackPos)
      lane = lane + 1
      stackPos = stackPos + 1
  else
    ()
