// prompt: verify an unproved array index in a Datalog table helper is rejected with IndexUnproved.
val Stigi.Datalog.NegTests.unguardedIndex {Stigi.NegTest}: Never =
  def readRaw(count: U64, column: Array[U64](count), index: U64) <lessOrEqual(0, index)>: U64 =
    column(index)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "array index without below proof must be rejected")
