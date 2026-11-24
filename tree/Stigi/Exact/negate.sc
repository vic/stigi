// prompt: negate an exact signed wide integer with overflow detection on minimum value.
def Stigi.Exact.negate(value: Stigi.Exact.IntWide): Stigi.Exact.Outcome =
  value match
    case WideInt(lowWord, highWord) =>
      val diffLow: U64 = U64.wrappingSubtract(0, lowWord)
      val borrowBit: U64 = if lowWord == 0 then 0 else 1
      @<below(borrowBit, 2)>
      val highBits: U64 = Stigi.Exact.signedToBits(highWord)
      val diffHighBits: U64 = U64.wrappingSubtract(U64.wrappingSubtract(0, highBits), borrowBit)
      val diffHigh: I64 = I64.ofU64(diffHighBits)
      if highWord < 0 && diffHigh < 0 then
        ExactOverflow
      else
        ExactValue(WideInt(diffLow, diffHigh))
