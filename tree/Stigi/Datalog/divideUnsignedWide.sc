// prompt: restoring binary division over unsigned 128-bit pairs for bitWidth steps.
def Stigi.Datalog.divideUnsignedWide(numLow: U64, numHigh: U64, denLow: U64, denHigh: U64, bitWidth: U64) <lessOrEqual(bitWidth, 128)>: Stigi.Datalog.Integer =
  var remLow: U64 = 0
  var remHigh: U64 = 0
  var quotLow: U64 = 0
  var quotHigh: U64 = 0
  var shiftLow: U64 = numLow
  var shiftHigh: U64 = numHigh
  var step: U64 = 0
  while step < bitWidth do
    @<lessOrEqual(step, bitWidth), decreases(bitWidth - step)>
    val topBit: U64 = U64.shiftRight(shiftHigh, 63)
    shiftHigh = U64.bitwiseOr(U64.shiftLeft(shiftHigh, 1), U64.shiftRight(shiftLow, 63))
    shiftLow = U64.shiftLeft(shiftLow, 1)
    remHigh = U64.bitwiseOr(U64.shiftLeft(remHigh, 1), U64.shiftRight(remLow, 63))
    remLow = U64.bitwiseOr(U64.shiftLeft(remLow, 1), topBit)
    quotHigh = U64.bitwiseOr(U64.shiftLeft(quotHigh, 1), U64.shiftRight(quotLow, 63))
    quotLow = U64.shiftLeft(quotLow, 1)
    if remHigh > denHigh || remHigh == denHigh && remLow >= denLow then
      val borrow: U64 = if remLow < denLow then 1 else 0
      remLow = U64.wrappingSubtract(remLow, denLow)
      remHigh = U64.wrappingSubtract(U64.wrappingSubtract(remHigh, denHigh), borrow)
      quotLow = U64.bitwiseOr(quotLow, 1)
    else
      ()
    step = step + 1
  ExactInteger(quotLow, quotHigh)
