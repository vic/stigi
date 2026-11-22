// prompt: compact per-callee verification summary preventing cross-unit extraction blowup.
pub enum Stigi.Extract.CalleeSummary:
  case Summary(entryFactCount: U64, exitFactCount: U64, propagatesTaint: Bool, isTotal: Bool)
