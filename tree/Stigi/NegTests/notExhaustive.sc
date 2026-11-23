// prompt: verify missing enum case in match rejects with NotExhaustive.
val Stigi.NegTests.notExhaustive {Stigi.NegTest}: Never =
  def Stigi.NegTests.badMatch(item: Stigi.Lookup): Bool =
    item match
      case Found(_) => true
  Stigi.NegTest.expect(Stigi.Code.NotExhaustive, "Missing case is omitted from match")
