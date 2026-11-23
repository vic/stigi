// prompt: verify unproved array index is rejected with IndexUnproved.
val Stigi.NegTests.indexUnproved {Stigi.NegTest}: Never =
  def Stigi.NegTests.badIndex(len: Stigi.ByteCap, arr: Array[U8](len), slot: U64): U8 =
    arr(slot)
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "slot is not proved below len")
