// prompt: closed result of reading a file into a bounded destination buffer.
pub enum Stigi.Posix.LoadResult:
  case LoadedBytes(byteCount: U64)
  case LoadFailed
  case LoadOverflow
