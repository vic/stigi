// prompt: writing a byte to the C output buffer without proving capacity bound is rejected.
val Stigi.Emit.NegTests.unguardedOutputWrite {Stigi.NegTest}: Never =
  def badWrite {Stigi.Mut} (outCap: U64, output: Array[U8](outCap), offset: U64, byteVal: U8) <lessOrEqual(0, outCap)>: Unit =
    output(offset) := byteVal
  Stigi.NegTest.expect(Stigi.Code.IndexUnproved, "output write offset is not proved below outCap")
