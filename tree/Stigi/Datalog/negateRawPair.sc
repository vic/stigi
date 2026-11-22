// prompt: exact two's-complement negation of a 128-bit unsigned magnitude pair.
def Stigi.Datalog.negateRawPair(lowWord: U64, highWord: U64) <lessOrEqual(0, lowWord), lessOrEqual(0, highWord)>: Stigi.Datalog.Integer =
  val negLow: U64 = U64.wrappingSubtract(0, lowWord)
  val borrow: U64 = if lowWord != 0 then 1 else 0
  val negHigh: U64 = U64.wrappingSubtract(U64.wrappingSubtract(0, highWord), borrow)
  ExactInteger(negLow, negHigh)
