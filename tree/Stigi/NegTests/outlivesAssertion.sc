// prompt: verify that an unbound assertion of outlives is rejected with AssertionUnproved.
val Stigi.NegTests.outlivesAssertion {Stigi.NegTest}: Never =
  def Stigi.NegTests.badOutlivesAssertion(item: U64, limit: U64) <lessOrEqual(0, item)>: Bool =
    @<outlives(item, limit)>
    false
  Stigi.NegTest.expect(Stigi.Code.AssertionUnproved, "no row proves a outlives atom")
