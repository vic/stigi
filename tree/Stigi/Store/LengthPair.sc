// prompt: pair two term indexes within one unit for sameLength or termOffset row formatting.
enum Stigi.Store.LengthPair:
  case LengthPair(leftIdx: U64, rightIdx: U64) <below(leftIdx, 256), below(rightIdx, 256)>
