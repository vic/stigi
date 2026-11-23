// prompt: detect whether a word slice folds to a forbidden tenet-gate token.
def Stigi.TenetGate.isBannedWord(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap)>: Bool =
  endPos - startPos match
    case 4 => if startPos + 3 < srcCap then Stigi.TenetGate.bannedFour(Stigi.TenetGate.toLowerByte(src(startPos)), Stigi.TenetGate.toLowerByte(src(startPos + 1)), Stigi.TenetGate.toLowerByte(src(startPos + 2)), Stigi.TenetGate.toLowerByte(src(startPos + 3))) else false
    case 5 => if startPos + 4 < srcCap then Stigi.TenetGate.bannedFive(Stigi.TenetGate.toLowerByte(src(startPos)), Stigi.TenetGate.toLowerByte(src(startPos + 1)), Stigi.TenetGate.toLowerByte(src(startPos + 2)), Stigi.TenetGate.toLowerByte(src(startPos + 3)), Stigi.TenetGate.toLowerByte(src(startPos + 4))) else false
    case _ => false
