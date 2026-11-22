// prompt: classify fact rows emitted by the single-pass SSA fact extractor.
pub enum Stigi.Extract.FactKind:
  case FactBelow
  case FactLessOrEqual
  case FactTermOffset
  case FactAddNode
  case FactSubtractNode
  case FactDominates
  case FactPhiNode
  case FactIndexRead
  case FactIndexWrite
  case FactDivisorNonZero
  case FactCapabilityState
  case FactDenial
  case FactDecreases
