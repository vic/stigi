// prompt: represent an exact signed 128-bit integer as low unsigned word and high signed word.
pub enum Stigi.Exact.IntWide:
  case WideInt(lowWord: U64, highWord: I64)
