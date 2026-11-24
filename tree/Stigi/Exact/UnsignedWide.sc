// prompt: hold an unsigned 128-bit magnitude as low and high 64-bit words.
pub enum Stigi.Exact.UnsignedWide:
  case UnsignedPair(lowWord: U64, highWord: U64)
