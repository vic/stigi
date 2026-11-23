// prompt: writing into a fixed 512-byte POSIX path buffer without proving index bound is rejected.
val Stigi.Posix.NegTests.unguardedPathWrite {Stigi.NegTest}: Never =
  def badPath {Stigi.Mut} (pathBuf: Array[U8](512), slot: U64) <lessOrEqual(0, slot)>: Unit =
    pathBuf(slot) := 0
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "path buffer slot is not proved below 512")
