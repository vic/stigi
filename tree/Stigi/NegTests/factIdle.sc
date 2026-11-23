// prompt: verify vacuous constant tautology fact rejects with FactIdle.
val Stigi.NegTests.factIdle {Stigi.NegTest}: Never =
  def Stigi.NegTests.badVacuous(flag: Bool) <lessOrEqual(0, 1)>: Bool = flag
  Stigi.NegTest.expect(Stigi.Code.FactIdle, "constant tautology lessOrEqual(0, 1) changes no verdict")
