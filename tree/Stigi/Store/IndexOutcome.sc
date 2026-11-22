// prompt: result of parsing a stored tree index file into columnar unit metadata.
pub enum Stigi.Store.IndexOutcome:
  case IndexParsed(unitCount: U64)
  case IndexCorrupt(lineNumber: U64)
