// prompt: outcome of emitting a deterministic ISO C11 translation unit.
pub enum Stigi.Emit.EmitOutcome:
  case Emitted(byteCount: U64, unitsEmitted: U64, lemmasErased: U64, spectreMasks: U64)
  case EmitBufferFull
  case EmitCrashLineViolation
