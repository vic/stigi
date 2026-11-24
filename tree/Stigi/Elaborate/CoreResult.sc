// prompt: represent the outcome of appending core rows that only read the scratch: the tables given back, or a full table, or invalid syntax.
enum Stigi.Elaborate.CoreResult:
  case CoreDone(core: Stigi.Core.Tables)
  case CoreFull
  case CoreInvalid
