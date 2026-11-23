// prompt: read the one lexical item that starts at a position of a source buffer.
def Stigi.Lex.nextToken(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, pos: U64, lineNum: U64, colNum: U64) <lessOrEqual(srcLen, srcCap), below(pos, srcLen)>: Stigi.Lex.Step =
  val nextPos: U64 = pos + 1
  val byte: U8 = src(pos)
  byte match
    case ' ' | '\t' | '\x0D' => Blank(nextPos)
    case '\n' => LineBreak(Token(TokNewline, pos, nextPos, lineNum, colNum), nextPos)
    case '/' if nextPos < srcLen && src(nextPos) == '/' =>
      var endPos: U64 = nextPos
      while endPos < srcLen && src(endPos) != '\n' do
        @<lessOrEqual(endPos, srcLen)>
        endPos = endPos + 1
      Blank(endPos)
    case 'a'..'z' | 'A'..'Z' | '_' =>
      val endPos: U64 = Stigi.Lex.identEnd(srcCap, src, srcLen, pos)
      Lexed(Token(Stigi.Lex.classifyWord(srcCap, src, pos, endPos), pos, endPos, lineNum, colNum), endPos)
    case '0'..'9' =>
      val endPos: U64 = Stigi.Lex.identEnd(srcCap, src, srcLen, pos)
      Lexed(Token(TokInt, pos, endPos, lineNum, colNum), endPos)
    case '"' =>
      Stigi.Lex.closingQuote(srcCap, src, srcLen, pos, nextPos, srcLen) match
        case Closed(startPos, endPos) => Lexed(Token(TokStr, startPos, endPos, lineNum, colNum), endPos)
        case Unclosed => Malformed
    case '\'' =>
      Stigi.Lex.closingQuote(srcCap, src, srcLen, pos, nextPos, srcLen) match
        case Closed(startPos, endPos) => Lexed(Token(TokChar, startPos, endPos, lineNum, colNum), endPos)
        case Unclosed => Malformed
    case _ =>
      val pair: Stigi.Lex.Punct = if nextPos < srcLen then Stigi.Lex.doublePunct(byte, src(nextPos)) else NotPunct
      pair match
        case Punct(kind) => Lexed(Token(kind, pos, pos + 2, lineNum, colNum), pos + 2)
        case NotPunct =>
          Stigi.Lex.singlePunct(byte) match
            case Punct(kind) => Lexed(Token(kind, pos, nextPos, lineNum, colNum), nextPos)
            case NotPunct => Malformed
