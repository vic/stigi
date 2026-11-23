// prompt: typed outcome of writing bytes to a POSIX file descriptor.
pub enum Stigi.Posix.WriteOutcome:
  case WroteBytes(byteCount: U64)
  case WriteFailed
