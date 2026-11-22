// prompt: writing an LEB128 byte without proving output offset bound is rejected.
val Stigi.Identity.NegTests.unguardedLebWrite {Stigi.NegTest}: Never =
  def writeByte {Stigi.Mut} (capacity: U64, output: Array[U8](capacity), cursor: U64, byte: U8) <lessOrEqual(0, capacity)>: Unit =
    output(cursor) := byte
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "output cursor is not bounded below capacity")
