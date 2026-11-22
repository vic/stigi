// prompt: outcome of an aligned bump allocation from a bounded Arena chunk.
pub enum Stigi.Arena.AllocOutcome:
  case Allocated(offset: U64, nextUsed: U64)
  case ArenaExhausted
