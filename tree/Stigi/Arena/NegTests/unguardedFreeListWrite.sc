// prompt: writing to a Heap free list without proving count below capacity is rejected.
val Stigi.Arena.NegTests.unguardedFreeListWrite {Stigi.NegTest}: Never =
  def badPush {Stigi.Mut} (cap: U64, freeSlots: Array[U64](cap), count: U64, blockIndex: U64) <lessOrEqual(0, cap)>: Unit =
    freeSlots(count) := blockIndex
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "free list write count is not proved below cap")
