// prompt: detect whether a word slice matches any forbidden tenet-gate token via ASCII codes.
def Stigi.TenetGate.isBannedWord(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap)>: Bool =
  val wordLen: U64 = endPos - startPos
  if wordLen == 4 && startPos + 3 < srcCap then
    val byteZero: U8 = Stigi.TenetGate.toLowerByte(src(startPos))
    val byteOne: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 1))
    val byteTwo: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 2))
    val byteThree: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 3))
    val hitFirst: Bool = byteZero == 116 && byteOne == 114 && byteTwo == 97 && byteThree == 112
    val hitSecond: Bool = byteZero == 116 && byteOne == 111 && byteTwo == 100 && byteThree == 111
    val hitThird: Bool = byteZero == 115 && byteOne == 107 && byteTwo == 105 && byteThree == 112
    val hitFourth: Bool = byteZero == 115 && byteOne == 116 && byteTwo == 117 && byteThree == 98
    hitFirst || hitSecond || hitThird || hitFourth
  else if wordLen == 5 && startPos + 4 < srcCap then
    val byteZero: U8 = Stigi.TenetGate.toLowerByte(src(startPos))
    val byteOne: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 1))
    val byteTwo: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 2))
    val byteThree: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 3))
    val byteFour: U8 = Stigi.TenetGate.toLowerByte(src(startPos + 4))
    val hitFirst: Bool = byteZero == 97 && byteOne == 98 && byteTwo == 111 && byteThree == 114 && byteFour == 116
    val hitSecond: Bool = byteZero == 112 && byteOne == 97 && byteTwo == 110 && byteThree == 105 && byteFour == 99
    val hitThird: Bool = byteZero == 102 && byteOne == 105 && byteTwo == 120 && byteThree == 109 && byteFour == 101
    val hitFourth: Bool = byteZero == 108 && byteOne == 97 && byteTwo == 116 && byteThree == 101 && byteFour == 114
    hitFirst || hitSecond || hitThird || hitFourth
  else false
