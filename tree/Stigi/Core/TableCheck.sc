// prompt: represent the outcome of validating a flat postorder Checked-IR table.
pub enum Stigi.Core.TableCheck:
  case TableValid(nodeCount: U64)
  case MalformedTable(badIndex: U64)
