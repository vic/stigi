// prompt: construct an exact signed wide integer from an unsigned 64-bit value.
def Stigi.Exact.fromUnsigned(value: U64): Stigi.Exact.IntWide =
  WideInt(value, 0)
