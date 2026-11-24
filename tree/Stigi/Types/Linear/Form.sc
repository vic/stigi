// prompt: represent a canonical linear length form as constant plus sorted term slice.
pub enum Stigi.Types.Linear.Form:
  case LinearForm(constant: Stigi.Exact.IntWide, startIdx: U64, termCount: U64)
