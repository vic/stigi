// prompt: where a probe of the row index ends: at the stored equal row, at a free slot, or with every slot probed.
enum Stigi.Datalog.Probe:
  case FoundAt(rowIdx: U64)
  case FreeAt(slot: U64)
  case ProbeFull
