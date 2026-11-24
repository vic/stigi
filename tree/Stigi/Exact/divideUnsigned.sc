// prompt: divide an unsigned 128-bit magnitude by an unsigned 64-bit divisor using up to 128 binary division steps.
pub def Stigi.Exact.divideUnsigned(magnitude: Stigi.Exact.UnsignedWide, divisor: U64, bitCount: U64) <lessOrEqual(bitCount, 128)>: Stigi.Exact.UnsignedDiv =
  val UnsignedPair(lowWord, highWord) = magnitude
  Stigi.Exact.divideStep(bitCount, lowWord, highWord, divisor, 0, 0, 0, 0, 0)
