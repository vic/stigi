// prompt: verify unproved addition overflow rejects with OverflowUnproved.
val Stigi.NegTests.overflowUnproved {Stigi.NegTest}: Never =
  def Stigi.NegTests.badAdd(left: U64, right: U64) <lessOrEqual(left, right)>: U64 =
    left + right
  Stigi.NegTest.expect(Stigi.Code.OverflowUnproved, "unbounded sum can exceed U64 max")
