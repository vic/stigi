// prompt: exact 128-bit signed subtraction rejecting signed overflow with IntegerOverflow.
pub def Stigi.Datalog.integerSubtract(left: Stigi.Datalog.Integer, right: Stigi.Datalog.Integer) <lessOrEqual(0, 1)>: Stigi.Datalog.IntegerOutcome =
  val signBit: U64 = 0x8000000000000000
  left match
    case ExactInteger(leftLow, leftHigh) =>
      right match
        case ExactInteger(rightLow, rightHigh) =>
          val diffLow: U64 = U64.wrappingSubtract(leftLow, rightLow)
          val borrow: U64 = if leftLow < rightLow then 1 else 0
          val diffHigh: U64 = U64.wrappingSubtract(U64.wrappingSubtract(leftHigh, rightHigh), borrow)
          val leftNegative: Bool = U64.bitwiseAnd(leftHigh, signBit) != 0
          val rightNegative: Bool = U64.bitwiseAnd(rightHigh, signBit) != 0
          val diffNegative: Bool = U64.bitwiseAnd(diffHigh, signBit) != 0
          if leftNegative != rightNegative && diffNegative != leftNegative then
            IntegerOverflow
          else
            IntegerValue(ExactInteger(diffLow, diffHigh))
