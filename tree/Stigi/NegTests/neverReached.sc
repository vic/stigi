// prompt: verify duplicate redundant case arm rejects with NeverReached.
val Stigi.NegTests.neverReached {Stigi.NegTest}: Never =
  def Stigi.NegTests.badDupArm(item: Stigi.Lookup): Bool =
    item match
      case Found(_) => true
      case Missing => false
      case Missing => true
  Stigi.NegTest.expect(Stigi.Code.NeverReached, "second Missing arm can never execute")
