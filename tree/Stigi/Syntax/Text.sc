// prompt: a run of source bytes of one unit, from its first byte to the byte after its last.
pub enum Stigi.Syntax.Text:
  case Text(first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>
