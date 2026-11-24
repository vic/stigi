// prompt: represent whether two linear length forms differ by an exact constant offset.
pub enum Stigi.Types.Linear.OffsetResult:
  case OffsetDelta(delta: Stigi.Exact.IntWide)
  case OffsetUnrelated
