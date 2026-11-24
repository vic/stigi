// prompt: the outcome of lowering the guarded arms of one match: the tables, whose arm mark starts the full select arms, and the scratch, or a full table, or invalid arms.
enum Stigi.Elaborate.GuardResult:
  case GuardsDone(core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch)
  case GuardsFull
  case GuardsInvalid
