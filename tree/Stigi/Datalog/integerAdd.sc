// prompt: exact 128-bit signed addition rejecting signed overflow with IntegerOverflow.
pub def Stigi.Datalog.integerAdd(left: Stigi.Datalog.Integer, right: Stigi.Datalog.Integer) <lessOrEqual(0, 1)>: Stigi.Datalog.IntegerOutcome =
  val signBit: U64 = 0x8000000000000000
  left match
    case ExactInteger(leftLow, leftHigh) =>
      right match
        case ExactInteger(rightLow, rightHigh) =>
          val sumLow: U64 = U64.wrappingAdd(leftLow, rightLow)
          val carry: U64 = if sumLow < leftLow then 1 else 0
          val sumHigh: U64 = U64.wrappingAdd(U64.wrappingAdd(leftHigh, rightHigh), carry)
          val leftNegative: Bool = U64.bitwiseAnd(leftHigh, signBit) != 0
          val rightNegative: Bool = U64.bitwiseAnd(rightHigh, signBit) != 0
          val sumNegative: Bool = U64.bitwiseAnd(sumHigh, signBit) != 0
          if leftNegative == rightNegative && sumNegative != leftNegative then
            IntegerOverflow
          else
            IntegerValue(ExactInteger(sumLow, sumHigh))
