// prompt: construct a 128-bit exact integer from a 64-bit unsigned word.
pub def Stigi.Datalog.integerFromUnsigned(word: U64) <lessOrEqual(0, word)>: Stigi.Datalog.Integer =
  ExactInteger(word, 0)
