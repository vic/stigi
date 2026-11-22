// prompt: exact 128-bit signed multiplication rejecting overflow outside signed 128-bit range.
pub def Stigi.Datalog.integerMultiply(left: Stigi.Datalog.Integer, right: Stigi.Datalog.Integer) <lessOrEqual(0, 1)>: Stigi.Datalog.IntegerOutcome =
  val signBit: U64 = 0x8000000000000000
  left match
    case ExactInteger(leftLow, leftHigh) =>
      right match
        case ExactInteger(rightLow, rightHigh) =>
          val leftNeg: Bool = U64.bitwiseAnd(leftHigh, signBit) != 0
          val rightNeg: Bool = U64.bitwiseAnd(rightHigh, signBit) != 0
          val resultNeg: Bool = leftNeg != rightNeg
          val absLeft: Stigi.Datalog.Integer = if leftNeg then Stigi.Datalog.negateRawPair(leftLow, leftHigh) else left
          val absRight: Stigi.Datalog.Integer = if rightNeg then Stigi.Datalog.negateRawPair(rightLow, rightHigh) else right
          absLeft match
            case ExactInteger(magLeftLow, magLeftHigh) =>
              absRight match
                case ExactInteger(magRightLow, magRightHigh) =>
                  if magLeftLow == 0 && magLeftHigh == 0 || magRightLow == 0 && magRightHigh == 0 then
                    IntegerValue(ExactInteger(0, 0))
                  else if magLeftHigh != 0 && magRightHigh != 0 then
                    IntegerOverflow
                  else
                    Stigi.Datalog.multiplyUnsignedWords(magLeftLow, magRightLow) match
                      case WideProduct(baseLow, baseHigh) =>
                        Stigi.Datalog.multiplyUnsignedWords(magLeftLow, magRightHigh) match
                          case WideProduct(crossLeftLow, crossLeftHigh) =>
                            Stigi.Datalog.multiplyUnsignedWords(magLeftHigh, magRightLow) match
                              case WideProduct(crossRightLow, crossRightHigh) =>
                                if crossLeftHigh != 0 || crossRightHigh != 0 then
                                  IntegerOverflow
                                else
                                  val crossSum: U64 = U64.wrappingAdd(crossLeftLow, crossRightLow)
                                  if crossSum < crossLeftLow then
                                    IntegerOverflow
                                  else
                                    val finalHigh: U64 = U64.wrappingAdd(baseHigh, crossSum)
                                    if finalHigh < baseHigh then
                                      IntegerOverflow
                                    else if resultNeg then
                                      if finalHigh > signBit || finalHigh == signBit && baseLow != 0 then
                                        IntegerOverflow
                                      else
                                        IntegerValue(Stigi.Datalog.negateRawPair(baseLow, finalHigh))
                                    else
                                      if finalHigh >= signBit then
                                        IntegerOverflow
                                      else
                                        IntegerValue(ExactInteger(baseLow, finalHigh))
