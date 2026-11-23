// prompt: verify that an unbound assertion of overwrites is rejected with AssertionUnproved.
val Stigi.NegTests.overwritesAssertion {Stigi.NegTest}: Never =
  def Stigi.NegTests.badOverwritesAssertion(item: U64, limit: U64) <lessOrEqual(0, item)>: Bool =
    @<overwrites(item)>
    false
  Stigi.NegTest.expect(Stigi.Code.AssertionUnproved, "no row proves a overwrites atom")
