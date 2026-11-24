// prompt: write the tokens from a cursor to the end of a source buffer using an iterative scan loop, then the end token.
def Stigi.Lex.scanFrom {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, tokCap: Stigi.TokenCap, outToks: Array[Stigi.Lex.Token](tokCap), at: Stigi.Lex.Cursor, remaining: U64) <lessOrEqual(srcLen, srcCap)>: Stigi.Lex.LexResult =
  val Cursor(startPos, startLine, startLineAt, startCount) = at
  var pos: U64 = startPos
  var lineNum: U64 = startLine
  var lineStart: U64 = startLineAt
  var tokCount: U64 = startCount
  var steps: U64 = remaining
  var outcome: Stigi.Lex.LexResult = LexDone(startCount)
  while pos < srcLen && 0 < steps && outcome is LexDone(_) do
    @<lessOrEqual(steps, remaining)>
    val colNum: U64 = if lineStart <= pos then pos - lineStart else 0
    Stigi.Lex.nextToken(srcCap, src, srcLen, pos, lineNum, colNum) match
      case Blank(endPos) => pos = endPos
      case Lexed(token, endPos) =>
        if tokCount < tokCap then
          outToks(tokCount) := token
          tokCount = tokCount + 1
          pos = endPos
        else outcome = LexOverflow
      case LineBreak(token, endPos) =>
        if tokCount < tokCap then
          outToks(tokCount) := token
          tokCount = tokCount + 1
          lineNum = if lineNum < Stigi.Limit.maxSourceBytes then lineNum + 1 else lineNum
          lineStart = endPos
          pos = endPos
        else outcome = LexOverflow
      case Malformed => outcome = LexSyntaxError(pos, lineNum)
    steps = steps - 1
  if !(outcome is LexDone(_)) then outcome else if tokCount < tokCap then
    outToks(tokCount) := Token(TokEnd, srcLen, srcLen, lineNum, 0)
    LexDone(tokCount + 1)
  else LexOverflow
