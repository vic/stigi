// prompt: writing unit metadata past maxUnits without proving bound is rejected.
val Stigi.Store.NegTests.unguardedIndexWrite {Stigi.NegTest}: Never =
  def badStore {Stigi.Mut} (maxUnits: U64, nameStarts: Array[U64](maxUnits), slot: U64) <lessOrEqual(0, maxUnits)>: Unit =
    nameStarts(slot) := 0
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "unit metadata slot is not proved below maxUnits")
