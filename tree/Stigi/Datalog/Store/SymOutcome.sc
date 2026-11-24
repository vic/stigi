// prompt: represent the outcome of interning a symbol into the symbol table.
enum Stigi.Datalog.Store.SymOutcome:
  case Interned(symNum: U64, newCount: U64, newPoolEnd: U64)
  case SymTableFull
