// prompt: type size and alignment descriptor carrying proved positive alignment.
pub enum Stigi.Types.Layout:
  case Layout(byteSize: U64, alignBytes: U64) <below(0, alignBytes), lessOrEqual(alignBytes, 64)>
