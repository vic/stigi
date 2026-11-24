// prompt: represent the outcome of emitting one core node: the tables and scratch given back, or a full table, or invalid syntax.
enum Stigi.Elaborate.EmitResult:
  case EmitDone(core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch)
  case EmitFull
  case EmitInvalid
