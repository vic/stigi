// prompt: represent a contiguous index range into a Checked-IR list table.
pub enum Stigi.Core.Span:
  case CoreSpan(first: U64, endPos: U64) <lessOrEqual(first, endPos)>
