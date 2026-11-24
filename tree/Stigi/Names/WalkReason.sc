// prompt: why a unit walk stopped early: its task stack was full, its scope was full, its node table was malformed, it ran out of steps, or its edge table was full.
pub enum Stigi.Names.WalkReason:
  case ReasonStack
  case ReasonScope
  case ReasonTable
  case ReasonSteps
  case ReasonEdges
