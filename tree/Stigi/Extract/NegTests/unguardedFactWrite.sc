// prompt: writing an extracted fact row without proving rowCap bound is rejected.
val Stigi.Extract.NegTests.unguardedFactWrite {Stigi.NegTest}: Never =
  def pushFact {Stigi.Mut} (rowCap: U64, rowKind: Array[U8](rowCap), slot: U64, kindTag: U8) <lessOrEqual(0, rowCap)>: Unit =
    rowKind(slot) := kindTag
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "fact row slot is not bounded below rowCap")
