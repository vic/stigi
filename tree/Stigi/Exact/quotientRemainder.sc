// prompt: compute exact signed wide integer quotient and remainder truncating toward zero.
pub def Stigi.Exact.quotientRemainder(dividend: Stigi.Exact.IntWide, divisor: Stigi.Exact.IntWide): Stigi.Exact.DivModOutcome =
  divisor match
    case WideInt(divLow, divHigh) =>
      if divLow == 0 && divHigh == 0 then
        DivModDivideByZero
      else
        dividend match
          case WideInt(numLowWord, numHighWord) =>
            val halfMin: I64 = -9223372036854775807
            val minHigh: I64 = halfMin - 1
            @<below(minHigh, 0)>
            if numLowWord == 0 && numHighWord == minHigh && divLow == 18446744073709551615 && divHigh == -1 then
              DivModOverflow
            else
              val dividendNeg: Bool = numHighWord < 0
              val divisorNeg: Bool = divHigh < 0
              val quotNeg: Bool = dividendNeg != divisorNeg
              Stigi.Exact.magnitudeOf(dividend) match
                case UnsignedPair(numMagLow, numMagHigh) =>
                  Stigi.Exact.magnitudeOf(divisor) match
                    case UnsignedPair(denMagLow, denMagHigh) =>
                      Stigi.Exact.divideStep(128, numMagLow, numMagHigh, denMagLow, denMagHigh, 0, 0, 0, 0) match
                        case UnsignedDivPair(quotLow, quotHigh, remLow, remHigh) =>
                          Stigi.Exact.applySign(UnsignedPair(quotLow, quotHigh), quotNeg) match
                            case ExactValue(signedQuot) =>
                              Stigi.Exact.applySign(UnsignedPair(remLow, remHigh), dividendNeg) match
                                case ExactValue(signedRem) => DivModValue(signedQuot, signedRem)
                                case ExactOverflow => DivModOverflow
                                case ExactDivideByZero => DivModDivideByZero
                            case ExactOverflow => DivModOverflow
                            case ExactDivideByZero => DivModDivideByZero
