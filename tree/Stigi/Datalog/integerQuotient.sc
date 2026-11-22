// prompt: exact 128-bit signed truncating division rejecting division by zero and minimum-divided-by-negative-one.
pub def Stigi.Datalog.integerQuotient(left: Stigi.Datalog.Integer, right: Stigi.Datalog.Integer) <lessOrEqual(0, 1)>: Stigi.Datalog.IntegerOutcome =
  val signBit: U64 = 0x8000000000000000
  left match
    case ExactInteger(leftLow, leftHigh) =>
      right match
        case ExactInteger(rightLow, rightHigh) =>
          if rightLow == 0 && rightHigh == 0 then
            IntegerOverflow
          else if leftLow == 0 && leftHigh == signBit && rightLow == 0xffffffffffffffff && rightHigh == 0xffffffffffffffff then
            IntegerOverflow
          else
            val leftNeg: Bool = U64.bitwiseAnd(leftHigh, signBit) != 0
            val rightNeg: Bool = U64.bitwiseAnd(rightHigh, signBit) != 0
            val resultNeg: Bool = leftNeg != rightNeg
            val absLeft: Stigi.Datalog.Integer = if leftNeg then Stigi.Datalog.negateRawPair(leftLow, leftHigh) else left
            val absRight: Stigi.Datalog.Integer = if rightNeg then Stigi.Datalog.negateRawPair(rightLow, rightHigh) else right
            absLeft match
              case ExactInteger(numLow, numHigh) =>
                absRight match
                  case ExactInteger(denLow, denHigh) =>
                    if numHigh == 0 && denHigh == 0 && denLow > 0 then
                      val fastQuot: U64 = numLow / denLow
                      if resultNeg then
                        IntegerValue(Stigi.Datalog.negateRawPair(fastQuot, 0))
                      else
                        IntegerValue(ExactInteger(fastQuot, 0))
                    else
                      val wideQuot: Stigi.Datalog.Integer = Stigi.Datalog.divideUnsignedWide(numLow, numHigh, denLow, denHigh, 128)
                      wideQuot match
                        case ExactInteger(quotLow, quotHigh) =>
                          if resultNeg then
                            IntegerValue(Stigi.Datalog.negateRawPair(quotLow, quotHigh))
                          else
                            IntegerValue(ExactInteger(quotLow, quotHigh))
