// prompt: probing a verdict cache slot without proving slotCap bound is rejected.
val Stigi.Door.NegTests.unguardedCacheSlot {Stigi.NegTest}: Never =
  def badProbe(slotCap: U64, usedSlots: Array[U8](slotCap), slot: U64) <lessOrEqual(0, slotCap)>: U8 =
    usedSlots(slot)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "cache slot is not bounded below slotCap")
