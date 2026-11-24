// prompt: apply a sign to an unsigned 128-bit magnitude and check signed range.
def Stigi.Exact.applySign(mag: Stigi.Exact.UnsignedWide, makeNeg: Bool): Stigi.Exact.Outcome =
  mag match
    case UnsignedPair(magLow, magHigh) =>
      if makeNeg then
        if magHigh > 9223372036854775808 then
          ExactOverflow
        else if magHigh == 9223372036854775808 && magLow > 0 then
          ExactOverflow
        else
          val resLow: U64 = U64.wrappingSubtract(0, magLow)
          val borrowBit: U64 = if magLow == 0 then 0 else 1
          @<below(borrowBit, 2)>
          val resHighBits: U64 = U64.wrappingSubtract(U64.wrappingSubtract(0, magHigh), borrowBit)
          ExactValue(WideInt(resLow, I64.ofU64(resHighBits)))
      else
        if magHigh >= 9223372036854775808 then
          ExactOverflow
        else
          ExactValue(WideInt(magLow, I64.ofU64(magHigh)))
