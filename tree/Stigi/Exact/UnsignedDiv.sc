// prompt: hold the unsigned 128-bit quotient and remainder of binary long division.
pub enum Stigi.Exact.UnsignedDiv:
  case UnsignedDivPair(quotLow: U64, quotHigh: U64, remLow: U64, remHigh: U64)
