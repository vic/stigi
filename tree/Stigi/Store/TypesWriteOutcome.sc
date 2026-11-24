// prompt: closed outcome of writing all type export fact files.
enum Stigi.Store.TypesWriteOutcome:
  case TypesWritten(rowCount: U64)
  case TypesWriteFailed(failure: Stigi.Store.FactsFailure)
