// prompt: push one syntax node index and visit phase onto the elaboration stack.
def Stigi.Elaborate.pushWork {Stigi.Mut} (workCap: U64, workNode: Array[U64](workCap), workPhase: Array[U8](workCap), topIdx: U64, synIdx: U64, phaseVal: U64) <lessOrEqual(topIdx, workCap), below(phaseVal, 256)>: Stigi.Elaborate.PushResult =
  if topIdx < workCap then
    val nextTop: U64 = topIdx + 1
    workNode(topIdx) := synIdx
    workPhase(topIdx) := U8.ofU64(phaseVal)
    PushDone(nextTop)
  else
    PushFull
