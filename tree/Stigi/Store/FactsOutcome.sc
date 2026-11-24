// prompt: closed outcome of writing facts files: every file was written, or the first failure is given.
enum Stigi.Store.FactsOutcome:
  case FactsWritten
  case FactsFailed(failure: Stigi.Store.FactsFailure)
