// prompt: typed outcome of opening a POSIX file descriptor.
pub enum Stigi.Posix.OpenOutcome:
  case FileOpened(descriptor: I32)
  case OpenFailed
