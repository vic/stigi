// prompt: verify two competing givens for Stigi.Equal[U64] reject with AmbiguousGivens.
val Stigi.NegTests.ambiguousGivens {Stigi.NegTest}: Never =
  given Stigi.NegTests.DupWord: Stigi.Equal[U64]:
    def equal(left: U64, right: U64): Bool = left == right
  def Stigi.NegTests.badAmb(num: U64) <lessOrEqual(0, num)>: Bool = Stigi.sameValue(num, num)
  Stigi.NegTest.expect(Stigi.Code.AmbiguousGivens, "two givens match Stigi.Equal[U64]")
