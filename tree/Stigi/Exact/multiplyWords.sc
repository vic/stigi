// prompt: multiply two unsigned 64-bit words into an exact unsigned 128-bit product.
def Stigi.Exact.multiplyWords(leftWord: U64, rightWord: U64): Stigi.Exact.UnsignedWide =
  val halfMask: U64 = 4294967295
  val leftLowHalf: U64 = U64.bitwiseAnd(leftWord, halfMask)
  @<below(leftLowHalf, 4294967296)>
  val leftHighHalf: U64 = U64.shiftRight(leftWord, 32)
  val rightLowHalf: U64 = U64.bitwiseAnd(rightWord, halfMask)
  val rightHighHalf: U64 = U64.shiftRight(rightWord, 32)
  val prodLowLow: U64 = U64.wrappingMultiply(leftLowHalf, rightLowHalf)
  val prodLowHigh: U64 = U64.wrappingMultiply(leftLowHalf, rightHighHalf)
  val prodHighLow: U64 = U64.wrappingMultiply(leftHighHalf, rightLowHalf)
  val prodHighHigh: U64 = U64.wrappingMultiply(leftHighHalf, rightHighHalf)
  val midSum: U64 = U64.wrappingAdd(U64.shiftRight(prodLowLow, 32), U64.bitwiseAnd(prodLowHigh, halfMask))
  val midFull: U64 = U64.wrappingAdd(midSum, U64.bitwiseAnd(prodHighLow, halfMask))
  val lowWord: U64 = U64.bitwiseOr(U64.bitwiseAnd(prodLowLow, halfMask), U64.shiftLeft(U64.bitwiseAnd(midFull, halfMask), 32))
  val highCarry: U64 = U64.wrappingAdd(U64.shiftRight(prodLowHigh, 32), U64.shiftRight(prodHighLow, 32))
  val highWord: U64 = U64.wrappingAdd(U64.wrappingAdd(prodHighHigh, highCarry), U64.shiftRight(midFull, 32))
  UnsignedPair(lowWord, highWord)
