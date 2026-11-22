// prompt: writing a normalized linear term without proving output capacity is rejected.
val Stigi.Types.NegTests.unguardedTermWrite {Stigi.NegTest}: Never =
  def storeTerm {Stigi.Mut} (outCap: U64, outBinders: Array[U64](outCap), pos: U64, binder: U64) <lessOrEqual(0, outCap)>: Unit =
    outBinders(pos) := binder
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "term write index is not bounded below outCap")
