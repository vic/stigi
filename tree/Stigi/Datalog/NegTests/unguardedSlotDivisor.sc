// prompt: verify an unproved slot count divisor is rejected with DivisorUnproved.
val Stigi.Datalog.NegTests.unguardedSlotDivisor {Stigi.NegTest}: Never =
  def badSlot(hashValue: U64, slotCount: U64) <lessOrEqual(0, hashValue)>: U64 =
    hashValue % slotCount
  Stigi.NegTest.expect(Stigi.Code.DivisorUnproved, "modulo by possibly zero slotCount must be rejected")
