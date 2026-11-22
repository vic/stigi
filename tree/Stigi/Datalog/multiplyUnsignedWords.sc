// prompt: multiply two 64-bit unsigned words into a 128-bit unsigned product with zero divisions.
def Stigi.Datalog.multiplyUnsignedWords(left: U64, right: U64) <lessOrEqual(0, left), lessOrEqual(0, right)>: Stigi.Datalog.UnsignedProduct =
  val limbMask: U64 = 0xffffffff
  val leftLow: U64 = U64.bitwiseAnd(left, limbMask)
  val leftHigh: U64 = U64.shiftRight(left, 32)
  val rightLow: U64 = U64.bitwiseAnd(right, limbMask)
  val rightHigh: U64 = U64.shiftRight(right, 32)
  val prodLow: U64 = U64.wrappingMultiply(leftLow, rightLow)
  val prodMidLeft: U64 = U64.wrappingMultiply(leftLow, rightHigh)
  val prodMidRight: U64 = U64.wrappingMultiply(leftHigh, rightLow)
  val prodHigh: U64 = U64.wrappingMultiply(leftHigh, rightHigh)
  val midFirst: U64 = U64.wrappingAdd(U64.shiftRight(prodLow, 32), U64.bitwiseAnd(prodMidLeft, limbMask))
  val midSum: U64 = U64.wrappingAdd(midFirst, U64.bitwiseAnd(prodMidRight, limbMask))
  val lowWord: U64 = U64.bitwiseOr(U64.shiftLeft(U64.bitwiseAnd(midSum, limbMask), 32), U64.bitwiseAnd(prodLow, limbMask))
  val highFirst: U64 = U64.wrappingAdd(prodHigh, U64.shiftRight(prodMidLeft, 32))
  val highSecond: U64 = U64.wrappingAdd(highFirst, U64.shiftRight(prodMidRight, 32))
  val highWord: U64 = U64.wrappingAdd(highSecond, U64.shiftRight(midSum, 32))
  WideProduct(lowWord, highWord)
