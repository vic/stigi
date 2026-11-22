// prompt: construct a 128-bit exact integer from a 64-bit signed word with sign extension.
pub def Stigi.Datalog.integerFromSigned(word: I64) <lessOrEqual(0, 1)>: Stigi.Datalog.Integer =
  if word < 0 then
    val positivePart: U64 = U64.wrappingSubtract(0, U64.ofI64(0 - (word + 1)))
    val lowBits: U64 = U64.wrappingSubtract(positivePart, 1)
    ExactInteger(lowBits, 0xffffffffffffffff)
  else
    ExactInteger(U64.ofI64(word), 0)
