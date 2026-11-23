// prompt: exact one-hundred-twenty-eight-bit signed integer stored as low unsigned and high signed words.
enum Stigi.Datalog.IntWide:
  case IntWide(lowWord: U64, highWord: I64)
