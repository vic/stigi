// prompt: represent the quotient and remainder outcome of exact wide integer division.
pub enum Stigi.Exact.DivModOutcome:
  case DivModValue(quotient: Stigi.Exact.IntWide, remainder: Stigi.Exact.IntWide)
  case DivModOverflow
  case DivModDivideByZero
