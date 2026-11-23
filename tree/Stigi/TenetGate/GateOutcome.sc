// prompt: closed result of running the Stigi tenet gate on unit source bytes.
pub enum Stigi.TenetGate.GateOutcome:
  case GateClean
  case GateMissingPrompt
  case GateBannedWord(offset: U64)
  case GateCrypticName(offset: U64)
  case GateTrailingDigit(offset: U64)
