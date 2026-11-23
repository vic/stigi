// prompt: scan a full L0 source buffer into a single contiguous typed Token array.
pub def Stigi.Lex.scanAll {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, tokCap: Stigi.TokenCap, outToks: Array[Stigi.Lex.Token](tokCap)) <lessOrEqual(srcLen, srcCap)>: Stigi.Lex.LexResult =
  var pos: U64 = 0
  var lineNum: U64 = 1
  var lineStart: U64 = 0
  var tokCount: U64 = 0
  var syntaxErr: Bool = false
  var overflowed: Bool = false
  var errPos: U64 = 0
  var errLine: U64 = 1
  while pos < srcLen && !syntaxErr && !overflowed do
    @<lessOrEqual(pos, srcLen), lessOrEqual(tokCount, tokCap), lessOrEqual(lineNum, Stigi.Limit.maxSourceBytes)>
    val curByte: U8 = src(pos)
    val colNum: U64 = if lineStart <= pos then pos - lineStart else 0
    val nextPos: U64 = pos + 1
    if curByte == 32 || curByte == 13 || curByte == 9 then
      pos = nextPos
    else if curByte == 10 then
      if tokCount < tokCap then
        outToks(tokCount) := Token(TokNewline, pos, nextPos, lineNum, colNum)
        tokCount = tokCount + 1
        pos = nextPos
        lineStart = nextPos
        if lineNum < Stigi.Limit.maxSourceBytes then lineNum = lineNum + 1 else ()
      else
        overflowed = true
    else if curByte == 47 && nextPos < srcLen && src(nextPos) == 47 then
      var scanPos: U64 = nextPos + 1
      while scanPos < srcLen && src(scanPos) != 10 do
        @<lessOrEqual(scanPos, srcLen)>
        scanPos = scanPos + 1
      pos = scanPos
    else if Stigi.Lex.isIdentStart(curByte) then
      var endPos: U64 = nextPos
      while endPos < srcLen && Stigi.Lex.isIdentBody(src(endPos)) do
        @<below(pos, endPos), lessOrEqual(endPos, srcLen)>
        endPos = endPos + 1
      if tokCount < tokCap then
        val kind: Stigi.Lex.TokenKind = Stigi.Lex.classifyWord(srcCap, src, pos, endPos)
        outToks(tokCount) := Token(kind, pos, endPos, lineNum, colNum)
        tokCount = tokCount + 1
        pos = endPos
      else
        overflowed = true
    else if Stigi.Lex.isDigitByte(curByte) then
      var endPos: U64 = nextPos
      while endPos < srcLen && Stigi.Lex.isIdentBody(src(endPos)) do
        @<lessOrEqual(pos, endPos), lessOrEqual(endPos, srcLen)>
        endPos = endPos + 1
      if tokCount < tokCap then
        outToks(tokCount) := Token(TokInt, pos, endPos, lineNum, colNum)
        tokCount = tokCount + 1
        pos = endPos
      else
        overflowed = true
    else if curByte == 34 then
      var endPos: U64 = nextPos
      var closed: Bool = false
      while endPos < srcLen && !closed do
        @<lessOrEqual(pos, endPos), lessOrEqual(endPos, srcLen)>
        val curChar: U8 = src(endPos)
        if curChar == 92 then
          val escNext: U64 = endPos + 2
          if escNext <= srcLen then endPos = escNext else endPos = srcLen
        else if curChar == 34 then
          endPos = endPos + 1
          closed = true
        else
          endPos = endPos + 1
      if !closed then
        syntaxErr = true
        errPos = pos
        errLine = lineNum
      else if tokCount < tokCap then
        outToks(tokCount) := Token(TokStr, pos, endPos, lineNum, colNum)
        tokCount = tokCount + 1
        pos = endPos
      else
        overflowed = true
    else if curByte == 39 then
      var endPos: U64 = nextPos
      var closed: Bool = false
      while endPos < srcLen && !closed do
        @<lessOrEqual(pos, endPos), lessOrEqual(endPos, srcLen)>
        val curChar: U8 = src(endPos)
        if curChar == 92 then
          val escNext: U64 = endPos + 2
          if escNext <= srcLen then endPos = escNext else endPos = srcLen
        else if curChar == 39 then
          endPos = endPos + 1
          closed = true
        else
          endPos = endPos + 1
      if !closed then
        syntaxErr = true
        errPos = pos
        errLine = lineNum
      else if tokCount < tokCap then
        outToks(tokCount) := Token(TokChar, pos, endPos, lineNum, colNum)
        tokCount = tokCount + 1
        pos = endPos
      else
        overflowed = true
    else
      val pairKind: Stigi.Lex.TokenKind = if nextPos < srcLen then Stigi.Lex.doublePunct(curByte, src(nextPos)) else TokEnd
      if pairKind == TokEnd then
        val oneKind: Stigi.Lex.TokenKind = Stigi.Lex.singlePunct(curByte)
        if oneKind == TokEnd then
          syntaxErr = true
          errPos = pos
          errLine = lineNum
        else if tokCount < tokCap then
          outToks(tokCount) := Token(oneKind, pos, nextPos, lineNum, colNum)
          tokCount = tokCount + 1
          pos = nextPos
        else
          overflowed = true
      else
        val twoEnd: U64 = pos + 2
        if tokCount < tokCap && twoEnd <= srcLen then
          outToks(tokCount) := Token(pairKind, pos, twoEnd, lineNum, colNum)
          tokCount = tokCount + 1
          pos = twoEnd
        else
          overflowed = true
  if syntaxErr then LexSyntaxError(errPos, errLine) else if overflowed then LexOverflow else if tokCount < tokCap then
    outToks(tokCount) := Token(TokEnd, srcLen, srcLen, lineNum, 0)
    LexDone(tokCount + 1)
  else LexOverflow
