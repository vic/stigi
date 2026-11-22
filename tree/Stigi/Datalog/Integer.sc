// prompt: exact signed 128-bit two's-complement integer stored as low and high unsigned 64-bit words.
pub enum Stigi.Datalog.Integer:
  case ExactInteger(lowWord: U64, highWord: U64)
