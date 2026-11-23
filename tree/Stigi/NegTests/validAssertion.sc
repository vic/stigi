// prompt: verify that an unbound assertion of valid is rejected with AssertionUnproved.
val Stigi.NegTests.validAssertion {Stigi.NegTest}: Never =
  def Stigi.NegTests.badValidAssertion(item: U64, limit: U64) <lessOrEqual(0, item)>: Bool =
    @<valid(item)>
    false
  Stigi.NegTest.expect(Stigi.Code.AssertionUnproved, "no row proves a valid atom")
