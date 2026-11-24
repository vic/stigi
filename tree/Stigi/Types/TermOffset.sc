// prompt: represent a strictly positive constant offset between two linear length forms sharing identical variable terms.
pub enum Stigi.Types.TermOffset:
  case TermOffsetPlus(constant: U64)
  case WithoutTermOffset
