// prompt: verify closing a Stigi.File twice rejects with OpNotAllowed.
val Stigi.NegTests.readAfterClose {Stigi.NegTest}: Never =
  def Stigi.NegTests.badFileUse(desc: I32) <lessOrEqual(0, desc)>: Bool =
    handle dupFile = Stigi.Posix.File(desc)
    val first: Stigi.File.CloseResult = dupFile.closeFile()
    val second: Stigi.File.CloseResult = dupFile.closeFile()
    (first, second) match
      case (ClosedClean, ClosedClean) => true
      case (ClosedClean, CloseFailed) => false
      case (CloseFailed, ClosedClean) => false
      case (CloseFailed, CloseFailed) => false
  Stigi.NegTest.expect(Stigi.Code.OpNotAllowed, "closeFile cannot run again in Closed state")
