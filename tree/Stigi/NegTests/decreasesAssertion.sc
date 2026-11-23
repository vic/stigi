// prompt: verify that an unbound assertion of decreases is rejected with AssertionUnproved.
val Stigi.NegTests.decreasesAssertion {Stigi.NegTest}: Never =
  def Stigi.NegTests.badDecreasesAssertion(item: U64, limit: U64) <lessOrEqual(0, item)>: Bool =
    @<decreases(item)>
    false
  Stigi.NegTest.expect(Stigi.Code.AssertionUnproved, "no row proves a decreases atom")
