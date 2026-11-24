// prompt: one open-addressed hash slot of the Datalog row index: empty, or holding the index of a stored row.
enum Stigi.Datalog.Slot:
  case Empty
  case Holds(rowIdx: U64)
