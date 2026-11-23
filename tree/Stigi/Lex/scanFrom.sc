// prompt: write the tokens from a cursor to the end of a source buffer, then the end token.
def Stigi.Lex.scanFrom {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, tokCap: Stigi.TokenCap, outToks: Array[Stigi.Lex.Token](tokCap), at: Stigi.Lex.Cursor, remaining: U64) <lessOrEqual(srcLen, srcCap), decreases(remaining)>: Stigi.Lex.LexResult =
  val Cursor(pos, lineNum, lineStart, tokCount) = at
  if pos < srcLen && remaining > 0 then
    val colNum: U64 = if lineStart <= pos then pos - lineStart else 0
    Stigi.Lex.nextToken(srcCap, src, srcLen, pos, lineNum, colNum) match
      case Blank(endPos) => Stigi.Lex.scanFrom(srcCap, src, srcLen, tokCap, outToks, Cursor(endPos, lineNum, lineStart, tokCount), remaining - 1)
      case Lexed(token, endPos) =>
        if tokCount < tokCap then
          outToks(tokCount) := token
          Stigi.Lex.scanFrom(srcCap, src, srcLen, tokCap, outToks, Cursor(endPos, lineNum, lineStart, tokCount + 1), remaining - 1)
        else LexOverflow
      case LineBreak(token, endPos) =>
        if tokCount < tokCap then
          outToks(tokCount) := token
          val nextLine: U64 = if lineNum < Stigi.Limit.maxSourceBytes then lineNum + 1 else lineNum
          Stigi.Lex.scanFrom(srcCap, src, srcLen, tokCap, outToks, Cursor(endPos, nextLine, endPos, tokCount + 1), remaining - 1)
        else LexOverflow
      case Malformed => LexSyntaxError(pos, lineNum)
  else if tokCount < tokCap then
    outToks(tokCount) := Token(TokEnd, srcLen, srcLen, lineNum, 0)
    LexDone(tokCount + 1)
  else LexOverflow
