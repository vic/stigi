// prompt: compute splitmix64 word mixing via pure sixty-four-bit arithmetic in C-DSL.
pub extern "c" def Stigi.Raw.mixWord(word: U64) <lessOrEqual(0, word)>: U64 =
  val first: U64 = U64.wrappingAdd(word, 0x9e3779b97f4a7c15)
  val second: U64 = U64.wrappingMultiply(U64.bitwiseXor(first, U64.shiftRight(first, 30)), 0xbf58476d1ce4e5b9)
  val third: U64 = U64.wrappingMultiply(U64.bitwiseXor(second, U64.shiftRight(second, 27)), 0x94d049bb133111eb)
  U64.bitwiseXor(third, U64.shiftRight(third, 31))
