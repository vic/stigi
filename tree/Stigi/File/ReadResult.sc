// prompt: closed result of reading bytes through Stigi.File.
pub enum Stigi.File.ReadResult:
  case ReadBytes(count: U64)
  case ReadEnd
  case ReadFailed
