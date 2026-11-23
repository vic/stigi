// prompt: typed outcome of reading bytes from a POSIX file descriptor.
pub enum Stigi.Posix.ReadOutcome:
  case ReadBytes(byteCount: U64)
  case ReadFailed
