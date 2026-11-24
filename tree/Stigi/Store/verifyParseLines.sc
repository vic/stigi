// prompt: check every unit named on the index lines from a line position, adding each outcome to the running counts.
def Stigi.Store.verifyParseLines {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, scratch: Stigi.Store.ParseScratch, indexLen: U64, walk: Stigi.Store.IndexWalk, tally: Stigi.Store.ParseTally, remaining: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(indexLen, 262144), decreases(remaining)>: Stigi.Store.ParseTally =
  val IndexWalk(linePos, unitCount) = walk
  if remaining < 1 || indexLen <= linePos then tally else
    val ParseScratch(_, indexBuf, _, _, _) = scratch
    val nameEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, linePos, true)
    val lineEnd: U64 = Stigi.Store.indexStop(indexBuf, indexLen, nameEnd, false)
    val nextLine: U64 = if lineEnd < indexLen then lineEnd + 1 else lineEnd
    if linePos < nameEnd && unitCount < Stigi.Limit.maxUnits then
      val counted: Stigi.Store.ParseTally = Stigi.Store.tallyUnit{region}(tally, Stigi.Store.parseOneUnit{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, linePos, nameEnd), scratch, linePos, nameEnd)
      Stigi.Store.verifyParseLines{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, indexLen, IndexWalk(nextLine, unitCount + 1), counted, remaining - 1)
    else Stigi.Store.verifyParseLines{region}(dirCap, dirBuf, dirStart, dirEnd, scratch, indexLen, IndexWalk(nextLine, unitCount), tally, remaining - 1)
