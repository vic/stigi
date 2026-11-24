// prompt: multiply two exact signed wide integers with signed overflow detection.
pub def Stigi.Exact.multiply(left: Stigi.Exact.IntWide, right: Stigi.Exact.IntWide): Stigi.Exact.Outcome =
  left match
    case WideInt(_, leftHigh) =>
      right match
        case WideInt(_, rightHigh) =>
          val leftNeg: Bool = leftHigh < 0
          val rightNeg: Bool = rightHigh < 0
          val resultNeg: Bool = leftNeg != rightNeg
          Stigi.Exact.magnitudeOf(left) match
            case UnsignedPair(leftMagLow, leftMagHigh) =>
              Stigi.Exact.magnitudeOf(right) match
                case UnsignedPair(rightMagLow, rightMagHigh) =>
                  if leftMagHigh > 0 && rightMagHigh > 0 then
                    ExactOverflow
                  else
                    Stigi.Exact.multiplyWords(leftMagLow, rightMagLow) match
                      case UnsignedPair(prodLow, prodHigh) =>
                        Stigi.Exact.multiplyWords(leftMagHigh, rightMagLow) match
                          case UnsignedPair(crossLeftLow, crossLeftHigh) =>
                            Stigi.Exact.multiplyWords(leftMagLow, rightMagHigh) match
                              case UnsignedPair(crossRightLow, crossRightHigh) =>
                                if crossLeftHigh > 0 || crossRightHigh > 0 then
                                  ExactOverflow
                                else
                                  val sumHighOne: U64 = U64.wrappingAdd(prodHigh, crossLeftLow)
                                  if sumHighOne < prodHigh then
                                    ExactOverflow
                                  else
                                    @<lessOrEqual(prodHigh, sumHighOne)>
                                    val sumHighTwo: U64 = U64.wrappingAdd(sumHighOne, crossRightLow)
                                    if sumHighTwo < sumHighOne then
                                      ExactOverflow
                                    else
                                      Stigi.Exact.applySign(UnsignedPair(prodLow, sumHighTwo), resultNeg)
