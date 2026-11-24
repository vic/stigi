// prompt: a run of bytes of a names pool, from its first byte to the byte after its last.
pub enum Stigi.Names.PoolRun:
  case PoolRun(first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>
