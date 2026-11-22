// prompt: scan the next token starting at or after startOffset in source.
pub def Stigi.Lex.nextToken(length: U64, source: Array[U8](length), startOffset: U64) <lessOrEqual(startOffset, length), below(length, 0x100000000)>: Stigi.Lex.ScanOutcome =
  var cursor: U64 = startOffset
  while cursor < length && source(cursor) == 32 do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, length), decreases(length - cursor)>
    cursor = cursor + 1
  if cursor < length then
    val head: U8 = source(cursor)
    if head == 9 then
      Stigi.Lex.ScanOutcome.ScanError(cursor, 1)
    else
      if head == 10 then
        Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenNewline, cursor, cursor + 1)
      else
        if Stigi.Lex.isUpperByte(head) then
          val endOffset: U64 = Stigi.Lex.stepIdentifier(length, source, cursor)
          Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenUpperIdent, cursor, endOffset)
        else
          if Stigi.Lex.isLowerByte(head) || head == 95 then
            val endOffset: U64 = Stigi.Lex.stepIdentifier(length, source, cursor)
            Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenIdent, cursor, endOffset)
          else
            if Stigi.Lex.isDigitByte(head) then
              val endOffset: U64 = Stigi.Lex.stepNumber(length, source, cursor)
              Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenInt, cursor, endOffset)
            else
              if head == 34 then
                val endOffset: U64 = Stigi.Lex.stepTextLiteral(length, source, cursor)
                Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenText, cursor, endOffset)
              else
                if head == 39 then
                  val nextPos: U64 = cursor + 1
                  if nextPos < length && source(nextPos) == 92 then
                    val closePos: U64 = cursor + 4
                    if closePos <= length then
                      Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenByte, cursor, closePos)
                    else
                      Stigi.Lex.ScanOutcome.ScanError(cursor, 2)
                  else
                    val closePos: U64 = cursor + 3
                    if closePos <= length then
                      Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenByte, cursor, closePos)
                    else
                      Stigi.Lex.ScanOutcome.ScanError(cursor, 2)
                else
                  val secondPos: U64 = cursor + 1
                  if head == 47 && secondPos < length && source(secondPos) == 47 then
                    val endOffset: U64 = Stigi.Lex.stepLineComment(length, source, cursor)
                    if cursor == 0 then
                      Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenPrompt, cursor, endOffset)
                    else
                      Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenComment, cursor, endOffset)
                  else
                    if secondPos < length && Stigi.Lex.isTwoByteOperator(head, source(secondPos)) then
                      Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenPunct, cursor, cursor + 2)
                    else
                      Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenPunct, cursor, cursor + 1)
  else
    Stigi.Lex.ScanOutcome.ScanToken(Stigi.Lex.TokenKind.TokenEnd, length, length)
