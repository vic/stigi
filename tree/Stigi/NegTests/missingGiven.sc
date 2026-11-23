// prompt: verify requesting an unresolved trait instance rejects with NoGiven.
val Stigi.NegTests.missingGiven {Stigi.NegTest}: Never =
  def Stigi.NegTests.badGiven(num: U64) <lessOrEqual(0, num)>: Bool =
    Stigi.sameValue(RunSuccess, RunSuccess) && num == 0
  Stigi.NegTest.expect(Stigi.Code.NoGiven, "no Stigi.Equal[Stigi.RunOutcome] given exists")
