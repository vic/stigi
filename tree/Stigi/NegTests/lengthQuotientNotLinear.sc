// prompt: verify non-constant divisor in an Array length is rejected with LengthNotLinear.
val Stigi.NegTests.lengthQuotientNotLinear {Stigi.NegTest}: Never =
  def Stigi.NegTests.badQuotient(total: U64, stride: U64, arr: Array[U8](total / stride)): U64 =
    total
  Stigi.NegTest.expect(Stigi.Code.LengthNotLinear, "total / stride is not linear")
