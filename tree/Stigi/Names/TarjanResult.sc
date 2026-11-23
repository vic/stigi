// prompt: closed result of linear-time iterative Tarjan strongly connected component analysis.
enum Stigi.Names.TarjanResult:
  case TarjanDone(compCount: U64)
  case TarjanOutOfMemory
