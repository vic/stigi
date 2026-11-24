// prompt: the state of one guarded-arm walk: still walking, a table is full, or the arms are invalid.
enum Stigi.Elaborate.GuardState:
  case Guarding
  case GuardsOverflowed
  case GuardsRejected
