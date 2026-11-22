// prompt: check whether one camelCase subword matches a disallowed four-byte or five-byte word.
def Stigi.TenetGate.isForbiddenSubword(length: U64, text: Array[U8](length), startOffset: U64, wordLength: U64) <lessOrEqual(startOffset, length), below(length, 0x100000000)>: Bool =
  if wordLength == 4 then
    val hitFirst: Bool = Stigi.TenetGate.matchFourBytes(length, text, startOffset, 116, 114, 97, 112) || Stigi.TenetGate.matchFourBytes(length, text, startOffset, 116, 111, 100, 111)
    val hitSecond: Bool = Stigi.TenetGate.matchFourBytes(length, text, startOffset, 115, 107, 105, 112) || Stigi.TenetGate.matchFourBytes(length, text, startOffset, 115, 116, 117, 98)
    hitFirst || hitSecond
  else
    if wordLength == 5 then
      val hitFirst: Bool = Stigi.TenetGate.matchFiveBytes(length, text, startOffset, 97, 98, 111, 114, 116) || Stigi.TenetGate.matchFiveBytes(length, text, startOffset, 102, 105, 120, 109, 101)
      val hitSecond: Bool = Stigi.TenetGate.matchFiveBytes(length, text, startOffset, 108, 97, 116, 101, 114) || Stigi.TenetGate.matchFiveBytes(length, text, startOffset, 112, 97, 110, 105, 99)
      hitFirst || hitSecond
    else
      false
