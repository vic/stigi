// prompt: closed failure of a names facts export, never a success.
enum Stigi.Store.FactsFailure:
  case FactsStopped(fault: Stigi.Store.CollectFault)
  case FileFailed(file: Stigi.Store.FactsFile, cause: Stigi.Store.FileCause)
  case FactsOutOfMemory
