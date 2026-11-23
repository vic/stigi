// prompt: verify effectful given requiring Stigi.File rejects with GivenRow.
val Stigi.NegTests.givenRow {Stigi.NegTest}: Never =
  given Stigi.NegTests.BadOrder {Stigi.File}: Stigi.Order[U8]:
    def lessThan(left: U8, right: U8): Bool = left < right
  def Stigi.NegTests.useOrder(num: U8) <lessOrEqual(0, num)>: Bool = Stigi.Order.lessThan(num, num)
  Stigi.NegTest.expect(Stigi.Code.GivenRow, "given clauses must remain pure and cannot carry Stigi.File")
