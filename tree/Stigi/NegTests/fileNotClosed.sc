// prompt: verify leaving an opened Stigi.File unclosed rejects with CapabilityNotEnded.
val Stigi.NegTests.fileNotClosed {Stigi.NegTest}: Never =
  def Stigi.NegTests.badLeakFile(desc: I32) <lessOrEqual(0, desc)>: Stigi.File.SizeResult =
    handle leaked = Stigi.Posix.File(desc)
    leaked.fileSize()
  Stigi.NegTest.expect(Stigi.Code.CapabilityNotEnded, "opened Stigi.File must end in Closed state")
