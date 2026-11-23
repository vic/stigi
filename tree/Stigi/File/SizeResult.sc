// prompt: closed result of querying file byte size through Stigi.File.
pub enum Stigi.File.SizeResult:
  case SizeBytes(total: U64)
  case SizeFailed
