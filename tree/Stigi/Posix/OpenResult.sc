// prompt: closed result of opening a POSIX file path.
enum Stigi.Posix.OpenResult:
  case Opened(descriptor: I32) <lessOrEqual(0, descriptor)>
  case OpenFailed
