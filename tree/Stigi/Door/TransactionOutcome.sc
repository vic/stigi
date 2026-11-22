// prompt: atomic transaction admission outcome for a dependency-closed component.
pub enum Stigi.Door.TransactionOutcome:
  case TransactionAdmitted(admittedCount: U64, cachedCount: U64, computedCount: U64)
  case TransactionRejected(failedUnit: U64, rejectCode: U64)
