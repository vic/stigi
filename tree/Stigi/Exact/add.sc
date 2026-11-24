// prompt: add two exact signed wide integers with signed overflow detection.
pub def Stigi.Exact.add(left: Stigi.Exact.IntWide, right: Stigi.Exact.IntWide): Stigi.Exact.Outcome =
  left match
    case WideInt(leftLow, leftHigh) =>
      right match
        case WideInt(rightLow, rightHigh) =>
          val sumLow: U64 = U64.wrappingAdd(leftLow, rightLow)
          val carryBit: U64 = if sumLow < leftLow then 1 else 0
          @<below(carryBit, 2)>
          val leftHighBits: U64 = Stigi.Exact.signedToBits(leftHigh)
          val rightHighBits: U64 = Stigi.Exact.signedToBits(rightHigh)
          val sumHighBits: U64 = U64.wrappingAdd(U64.wrappingAdd(leftHighBits, rightHighBits), carryBit)
          val sumHigh: I64 = I64.ofU64(sumHighBits)
          val leftNeg: Bool = leftHigh < 0
          val rightNeg: Bool = rightHigh < 0
          val sumNeg: Bool = sumHigh < 0
          if leftNeg == rightNeg && sumNeg != leftNeg then
            ExactOverflow
          else
            ExactValue(WideInt(sumLow, sumHigh))
