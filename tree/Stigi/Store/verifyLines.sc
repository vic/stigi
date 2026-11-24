// prompt: check every unit named on the index lines from a line position, stopping at the first failure.
def Stigi.Store.verifyLines {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, scratch: Stigi.Store.Scratch, indexLen: U64, walk: Stigi.Store.IndexWalk, remaining: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(indexLen, 262144), decreases(remaining)>: Stigi.RunOutcome =
  val IndexWalk(linePos, unitCount) = walk
  if remaining < 1 || indexLen <= linePos then
    if unitCount == 0 then RunDoorRejected else RunSuccess
  else
    val Scratch(_, indexBuf, _, _, _, _, _, _, _) = scratch
    val nameEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, linePos, true)
    val lineEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, nameEnd, false)
    val nextLine: U64 = if lineEnd < indexLen then lineEnd + 1 else lineEnd
    if linePos < nameEnd then
      if unitCount < Stigi.Limit.maxUnits then
        val outcome: Stigi.RunOutcome = Stigi.Store.verifyIndexed{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, linePos, nameEnd)
        if outcome is RunSuccess then Stigi.Store.verifyLines{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, indexLen, IndexWalk(nextLine, unitCount + 1), remaining - 1) else outcome
      else RunFileError
    else Stigi.Store.verifyLines{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, indexLen, IndexWalk(nextLine, unitCount), remaining - 1)
