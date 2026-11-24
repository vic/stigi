// prompt: the row total after deriving, or the row budget exhausted.
enum Stigi.Datalog.Derive:
  case Derived(total: U64)
  case DeriveFull
