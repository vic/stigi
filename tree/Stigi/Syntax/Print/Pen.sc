// prompt: the position of a printer in the output table, or why it stopped writing.
pub enum Stigi.Syntax.Print.Pen:
  case Pen(at: U64) <lessOrEqual(at, Stigi.Limit.maxUnitBytes)>
  case PenOverflow
  case PenFailed(fault: Stigi.Syntax.Print.Fault)
