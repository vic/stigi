// prompt: closed SSA fact relation kind emitted by the AST fact extractor.
pub enum Stigi.Extract.FactKind:
  case FactBelow
  case FactLessEqual
  case FactIndexOwed
  case FactCall
  case FactAlloc
  case FactFreeEdge
  case FactStackObject
  case FactGivenCheck
  case FactCapState
  case FactTaint
