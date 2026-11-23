// prompt: find the first top-level token matching targetText inside startTok to endTok.
pub def Stigi.Emit.findTokenInLine(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, startTok: U64, endTok: U64, targetLen: U64, targetText: Array[U8](targetLen)) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCount), below(srcCap, 0x100000000), below(targetLen, 64)>: U64 <lessOrEqual(startTok, result), lessOrEqual(result, endTok)> =
  var cursor: U64 = startTok
  var foundPos: U64 = endTok
  var stepBudget: U64 = 0
  while cursor < endTok && foundPos == endTok && stepBudget < tokCount do
    @<lessOrEqual(startTok, cursor), lessOrEqual(cursor, endTok), lessOrEqual(startTok, foundPos), lessOrEqual(foundPos, endTok), lessOrEqual(stepBudget, tokCount), decreases(tokCount - stepBudget)>
    if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, targetLen, targetText) then
      foundPos = cursor
      cursor = cursor + 1
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "(") then
      val afterGroup: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, 40, 41)
      cursor = if afterGroup <= endTok then afterGroup else endTok
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "[") then
      val afterGroup: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, 91, 93)
      cursor = if afterGroup <= endTok then afterGroup else endTok
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "{") then
      val afterGroup: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, 123, 125)
      cursor = if afterGroup <= endTok then afterGroup else endTok
    else
      cursor = cursor + 1
    stepBudget = stepBudget + 1
  foundPos
