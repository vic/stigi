// prompt: unguarded read into the sixteen-word BLAKE3 state array is rejected.
val Stigi.Hash.NegTests.unguardedStateRead {Stigi.NegTest}: Never =
  def peekState(state: Array[U32](16), lane: U64) <lessOrEqual(0, lane)>: U32 =
    state(lane)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "state lane index is not bounded below sixteen")
