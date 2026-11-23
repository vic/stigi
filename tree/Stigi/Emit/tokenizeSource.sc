// prompt: tokenize an L0 source slice into flat columnar token arrays omitting comments and blank lines.
pub def Stigi.Emit.tokenizeSource {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), srcStart: U64, srcEnd: U64, maxToks: U64, tokKind: Array[U8](maxToks), tokStart: Array[U64](maxToks), tokEnd: Array[U64](maxToks), tokLine: Array[U64](maxToks), tokCol: Array[U64](maxToks)) <lessOrEqual(srcStart, srcEnd), lessOrEqual(srcEnd, srcCap), below(srcCap, 0x100000000), below(maxToks, 65536)>: U64 <lessOrEqual(result, maxToks)> =
  var cursor: U64 = srcStart
  var lineStart: U64 = srcStart
  var lineOffset: U64 = 0
  var tokCount: U64 = 0
  var stepBudget: U64 = 0
  var lineHasTok: Bool = false
  val spanLen: U64 = srcEnd - srcStart
  while cursor < srcEnd && stepBudget < spanLen do
    @<lessOrEqual(srcStart, lineStart), lessOrEqual(lineStart, cursor), lessOrEqual(cursor, srcEnd), lessOrEqual(tokCount, maxToks), lessOrEqual(stepBudget, spanLen), lessOrEqual(lineOffset, stepBudget), decreases(spanLen - stepBudget)>
    val nextStep: U64 = stepBudget + 1
    val head: U8 = srcBytes(cursor)
    val nextPos: U64 = cursor + 1
    if head == 32 || head == 13 then
      cursor = nextPos
    else if head == 10 then
      if lineHasTok && tokCount < maxToks then
        tokKind(tokCount) := 7
        tokStart(tokCount) := cursor
        tokEnd(tokCount) := nextPos
        tokLine(tokCount) := lineOffset + 1
        tokCol(tokCount) := cursor - lineStart
        tokCount = tokCount + 1
        lineHasTok = false
      else
        ()
      cursor = nextPos
    else if head == 47 && nextPos < srcEnd && srcBytes(nextPos) == 47 then
      var commentEnd: U64 = nextPos + 1
      while commentEnd < srcEnd && srcBytes(commentEnd) != 10 do
        @<lessOrEqual(cursor, commentEnd), lessOrEqual(commentEnd, srcEnd), decreases(srcEnd - commentEnd)>
        commentEnd = commentEnd + 1
      cursor = commentEnd
    else if Stigi.Lex.isLetterByte(head) || head == 95 then
      val isUpper: Bool = head >= 65 && head <= 90
      val startPos: U64 = cursor
      var endPos: U64 = nextPos
      while endPos < srcEnd && Stigi.Emit.isNameBodyByte(srcBytes(endPos)) do
        @<lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcEnd), decreases(srcEnd - endPos)>
        endPos = endPos + 1
      if tokCount < maxToks then
        tokKind(tokCount) := if isUpper then 2 else 1
        tokStart(tokCount) := startPos
        tokEnd(tokCount) := endPos
        tokLine(tokCount) := lineOffset + 1
        tokCol(tokCount) := startPos - lineStart
        tokCount = tokCount + 1
        lineHasTok = true
      else
        ()
      cursor = endPos
    else if Stigi.Lex.isDigitByte(head) then
      val startPos: U64 = cursor
      var endPos: U64 = nextPos
      while endPos < srcEnd && Stigi.Emit.isNumberBodyByte(srcBytes(endPos)) do
        @<lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcEnd), decreases(srcEnd - endPos)>
        endPos = endPos + 1
      if tokCount < maxToks then
        tokKind(tokCount) := 3
        tokStart(tokCount) := startPos
        tokEnd(tokCount) := endPos
        tokLine(tokCount) := lineOffset + 1
        tokCol(tokCount) := startPos - lineStart
        tokCount = tokCount + 1
        lineHasTok = true
      else
        ()
      cursor = endPos
    else if head == 34 then
      val startPos: U64 = cursor
      var endPos: U64 = nextPos
      var escaped: Bool = false
      var closed: Bool = false
      while endPos < srcEnd && !closed do
        @<lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcEnd), decreases(srcEnd - endPos)>
        val byteVal: U8 = srcBytes(endPos)
        if escaped then
          escaped = false
          endPos = endPos + 1
        else if byteVal == 92 then
          escaped = true
          endPos = endPos + 1
        else if byteVal == 34 then
          endPos = endPos + 1
          closed = true
        else
          endPos = endPos + 1
      if tokCount < maxToks then
        tokKind(tokCount) := 5
        tokStart(tokCount) := startPos
        tokEnd(tokCount) := endPos
        tokLine(tokCount) := lineOffset + 1
        tokCol(tokCount) := startPos - lineStart
        tokCount = tokCount + 1
        lineHasTok = true
      else
        ()
      cursor = endPos
    else if head == 39 then
      val startPos: U64 = cursor
      val endPos: U64 = if nextPos < srcEnd && cursor + 4 <= srcEnd && srcBytes(nextPos) == 92 then cursor + 4 else if cursor + 3 <= srcEnd then cursor + 3 else srcEnd
      if tokCount < maxToks then
        tokKind(tokCount) := 4
        tokStart(tokCount) := startPos
        tokEnd(tokCount) := endPos
        tokLine(tokCount) := lineOffset + 1
        tokCol(tokCount) := startPos - lineStart
        tokCount = tokCount + 1
        lineHasTok = true
      else
        ()
      cursor = endPos
    else
      val startPos: U64 = cursor
      val endPos: U64 = if nextPos < srcEnd then if Stigi.Emit.isTwoBytePunct(head, srcBytes(nextPos)) then nextPos + 1 else nextPos else nextPos
      if tokCount < maxToks then
        tokKind(tokCount) := 6
        tokStart(tokCount) := startPos
        tokEnd(tokCount) := endPos
        tokLine(tokCount) := lineOffset + 1
        tokCol(tokCount) := startPos - lineStart
        tokCount = tokCount + 1
        lineHasTok = true
      else
        ()
      cursor = endPos
    lineStart = if head == 10 then cursor else lineStart
    lineOffset = if head == 10 then lineOffset + 1 else lineOffset
    stepBudget = nextStep
  if lineHasTok && tokCount < maxToks then
    tokKind(tokCount) := 7
    tokStart(tokCount) := srcEnd
    tokEnd(tokCount) := srcEnd
    tokLine(tokCount) := lineOffset + 1
    tokCol(tokCount) := 0
    tokCount + 1
  else
    tokCount
