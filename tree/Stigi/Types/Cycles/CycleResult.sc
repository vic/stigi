// prompt: represent the verdict of recursive-type cycle detection over type declarations.
pub enum Stigi.Types.Cycles.CycleResult:
  case AcyclicTypes
  case RecursiveCycle(fromType: U64, toType: U64)
  case CycleStackFull
