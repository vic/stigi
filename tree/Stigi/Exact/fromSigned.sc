// prompt: construct an exact signed wide integer from a signed 64-bit value.
def Stigi.Exact.fromSigned(value: I64): Stigi.Exact.IntWide =
  val lowWord: U64 = Stigi.Exact.signedToBits(value)
  val highWord: I64 = if value < 0 then -1 else 0
  @<lessOrEqual(-1, highWord)>
  WideInt(lowWord, highWord)
