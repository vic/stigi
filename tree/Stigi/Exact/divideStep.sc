// prompt: execute one tail-recursive restoring binary long division step over 128 bits.
def Stigi.Exact.divideStep(bitStep: U64, numLow: U64, numHigh: U64, denLow: U64, denHigh: U64, quotLow: U64, quotHigh: U64, remLow: U64, remHigh: U64) <lessOrEqual(bitStep, 128), decreases(bitStep)>: Stigi.Exact.UnsignedDiv =
  if bitStep < 1 then
    UnsignedDivPair(quotLow, quotHigh, remLow, remHigh)
  else
    val nextStep: U64 = bitStep - 1
    val curBit: U64 =
      if nextStep >= 64 then
        val shiftAmt: U64 = nextStep - 64
        U64.bitwiseAnd(U64.shiftRight(numHigh, shiftAmt), 1)
      else
        U64.bitwiseAnd(U64.shiftRight(numLow, nextStep), 1)
    val remCarry: U64 = U64.shiftRight(remLow, 63)
    val shiftedRemHigh: U64 = U64.bitwiseOr(U64.shiftLeft(remHigh, 1), remCarry)
    val shiftedRemLow: U64 = U64.bitwiseOr(U64.shiftLeft(remLow, 1), curBit)
    val quotCarry: U64 = U64.shiftRight(quotLow, 63)
    val shiftedQuotHigh: U64 = U64.bitwiseOr(U64.shiftLeft(quotHigh, 1), quotCarry)
    val shiftedQuotLow: U64 = U64.shiftLeft(quotLow, 1)
    val fitsDen: Bool =
      if shiftedRemHigh > denHigh then
        true
      else if shiftedRemHigh < denHigh then
        false
      else
        shiftedRemLow >= denLow
    if fitsDen then
      val borrowBit: U64 = if shiftedRemLow < denLow then 1 else 0
      val newRemLow: U64 = U64.wrappingSubtract(shiftedRemLow, denLow)
      val newRemHigh: U64 = U64.wrappingSubtract(U64.wrappingSubtract(shiftedRemHigh, denHigh), borrowBit)
      val newQuotLow: U64 = U64.bitwiseOr(shiftedQuotLow, 1)
      Stigi.Exact.divideStep(nextStep, numLow, numHigh, denLow, denHigh, newQuotLow, shiftedQuotHigh, newRemLow, newRemHigh)
    else
      Stigi.Exact.divideStep(nextStep, numLow, numHigh, denLow, denHigh, shiftedQuotLow, shiftedQuotHigh, shiftedRemLow, shiftedRemHigh)
