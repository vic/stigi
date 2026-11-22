// prompt: compare two exact 128-bit signed integers returning negative one, zero, or one.
pub def Stigi.Datalog.integerCompare(left: Stigi.Datalog.Integer, right: Stigi.Datalog.Integer) <lessOrEqual(0, 1)>: I32 =
  val signBit: U64 = 0x8000000000000000
  left match
    case ExactInteger(leftLow, leftHigh) =>
      right match
        case ExactInteger(rightLow, rightHigh) =>
          val leftKey: U64 = U64.bitwiseXor(leftHigh, signBit)
          val rightKey: U64 = U64.bitwiseXor(rightHigh, signBit)
          if leftKey < rightKey then -1 else if leftKey > rightKey then 1 else if leftLow < rightLow then -1 else if leftLow > rightLow then 1 else 0
