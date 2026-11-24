// prompt: verify non-linear product of two binders in an Array length is rejected with LengthNotLinear.
val Stigi.NegTests.lengthNotLinear {Stigi.NegTest}: Never =
  def Stigi.NegTests.badProduct(rows: U64, cols: U64, arr: Array[U8](rows * cols)): U64 =
    rows
  Stigi.NegTest.expect(Stigi.Code.LengthNotLinear, "rows * cols is not linear")
