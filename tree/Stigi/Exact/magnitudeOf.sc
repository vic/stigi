// prompt: compute the unsigned 128-bit magnitude of a signed wide integer.
def Stigi.Exact.magnitudeOf(value: Stigi.Exact.IntWide): Stigi.Exact.UnsignedWide =
  value match
    case WideInt(lowWord, highWord) =>
      if highWord < 0 then
        val negLow: U64 = U64.wrappingSubtract(0, lowWord)
        val borrowBit: U64 = if lowWord == 0 then 0 else 1
        val highBits: U64 = Stigi.Exact.signedToBits(highWord)
        val negHigh: U64 = U64.wrappingSubtract(U64.wrappingSubtract(0, highBits), borrowBit)
        UnsignedPair(negLow, negHigh)
      else
        @<lessOrEqual(0, highWord)>
        UnsignedPair(lowWord, U64.ofI64(highWord))
