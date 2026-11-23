// prompt: closed result of writing bytes through Stigi.File.
pub enum Stigi.File.WriteResult:
  case WroteBytes(count: U64)
  case WriteFailed
