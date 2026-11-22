// prompt: validate one non-empty identifier against trailing digits, cryptic short words, and disallowed words.
pub def Stigi.TenetGate.checkIdentifier(length: U64, text: Array[U8](length)) <below(0, length), below(length, 0x100000000)>: Stigi.TenetGate.Finding =
  val lastPos: U64 = length - 1
  if Stigi.Lex.isDigitByte(text(lastPos)) then
    Stigi.TenetGate.Finding.FindingTrailingDigit(lastPos)
  else
    var cursor: U64 = 0
    var finding: Stigi.TenetGate.Finding = Stigi.TenetGate.Finding.FindingClean
    var failed: Bool = false
    while cursor < length && !failed do
      @<lessOrEqual(cursor, length), decreases(length - cursor)>
      val wordStart: U64 = cursor
      cursor = cursor + 1
      while cursor < length && Stigi.TenetGate.continuesSubword(text(cursor)) do
        @<lessOrEqual(wordStart, cursor), lessOrEqual(cursor, length), decreases(length - cursor)>
        cursor = cursor + 1
      val wordLen: U64 = cursor - wordStart
      if wordLen == 1 then
        finding = Stigi.TenetGate.Finding.FindingCrypticName(wordStart)
        failed = true
      else
        if wordLen == 2 then
          val secondPos: U64 = wordStart + 1
          if secondPos < length then
            val firstLower: U8 = Stigi.TenetGate.toLowerByte(text(wordStart))
            val secondLower: U8 = Stigi.TenetGate.toLowerByte(text(secondPos))
            if !Stigi.TenetGate.isAllowedTwoLetter(firstLower, secondLower) then
              finding = Stigi.TenetGate.Finding.FindingCrypticName(wordStart)
              failed = true
            else
              failed = false
          else
            failed = false
        else
          if Stigi.TenetGate.isForbiddenSubword(length, text, wordStart, wordLen) then
            finding = Stigi.TenetGate.Finding.FindingForbiddenWord(wordStart)
            failed = true
          else
            failed = false
    finding
