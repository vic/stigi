// prompt: subtract two exact signed wide integers with signed overflow detection.
pub def Stigi.Exact.subtract(left: Stigi.Exact.IntWide, right: Stigi.Exact.IntWide): Stigi.Exact.Outcome =
  left match
    case WideInt(leftLow, leftHigh) =>
      right match
        case WideInt(rightLow, rightHigh) =>
          val diffLow: U64 = U64.wrappingSubtract(leftLow, rightLow)
          val borrowBit: U64 = if leftLow < rightLow then 1 else 0
          @<below(borrowBit, 2)>
          val leftHighBits: U64 = Stigi.Exact.signedToBits(leftHigh)
          val rightHighBits: U64 = Stigi.Exact.signedToBits(rightHigh)
          val diffHighBits: U64 = U64.wrappingSubtract(U64.wrappingSubtract(leftHighBits, rightHighBits), borrowBit)
          val diffHigh: I64 = I64.ofU64(diffHighBits)
          val leftNeg: Bool = leftHigh < 0
          val rightNeg: Bool = rightHigh < 0
          val diffNeg: Bool = diffHigh < 0
          if leftNeg != rightNeg && diffNeg != leftNeg then
            ExactOverflow
          else
            ExactValue(WideInt(diffLow, diffHigh))
