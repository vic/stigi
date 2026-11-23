// prompt: check whether another sibling statement follows a block at the same indentation column.
pub def Stigi.Emit.hasSiblingAfterBlock(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCol: Array[U64](tokCap), tokCount: U64, afterLineEnd: U64, blockCol: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(afterLineEnd, tokCount), below(srcCap, 0x100000000)>: Bool =
  var cursor: U64 = afterLineEnd
  var foundSibling: Bool = false
  var doneScan: Bool = false
  var stepBudget: U64 = 0
  while cursor < tokCount && !doneScan && stepBudget < tokCount do
    @<lessOrEqual(cursor, tokCount), lessOrEqual(stepBudget, tokCount), decreases(tokCount - stepBudget)>
    if tokKind(cursor) == 7 then
      cursor = cursor + 1
    else
      val lineCol: U64 = tokCol(cursor)
      if lineCol < blockCol then
        doneScan = true
      else if lineCol == blockCol then
        val isElse: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "else")
        val isCase: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "case")
        if !isElse && !isCase then
          foundSibling = true
          doneScan = true
        else
          cursor = Stigi.Emit.findLineEnd(tokCap, tokKind, tokCount, cursor)
      else
        cursor = Stigi.Emit.findLineEnd(tokCap, tokKind, tokCount, cursor)
    stepBudget = stepBudget + 1
  foundSibling
