// prompt: exact one-hundred-twenty-eight-bit addition with carry propagation for Datalog arithmetic.
def Stigi.Datalog.wideAdd(left: Stigi.Datalog.IntWide, right: Stigi.Datalog.IntWide, maxHigh: I64) <lessOrEqual(0, maxHigh), lessOrEqual(maxHigh, 1000000)>: Stigi.Datalog.IntWide =
  val IntWide(leftLow, leftHigh) = left
  val IntWide(rightLow, rightHigh) = right
  val sumLow: U64 = U64.wrappingAdd(leftLow, rightLow)
  val carry: I64 = if sumLow < leftLow then 1 else 0
  val boundedLeft: I64 = if leftHigh >= -1000000 && leftHigh <= maxHigh then leftHigh else 0
  val boundedRight: I64 = if rightHigh >= -1000000 && rightHigh <= maxHigh then rightHigh else 0
  val sumHigh: I64 = boundedLeft + boundedRight + carry
  IntWide(sumLow, sumHigh)
