// prompt: result of extracting SSA fact rows and deduplicated literals for a unit body.
pub enum Stigi.Extract.ExtractOutcome:
  case Extracted(factCount: U64, literalCount: U64)
  case ExtractBudgetExceeded
