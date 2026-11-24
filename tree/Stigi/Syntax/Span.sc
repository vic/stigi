// prompt: a run of entries of the list table of one unit, from its first entry to the entry after its last.
pub enum Stigi.Syntax.Span:
  case Span(first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitNodes)>
