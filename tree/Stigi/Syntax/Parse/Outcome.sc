// prompt: the outcome of parsing one unit: its root node with the node and token counts, or why it was rejected.
pub enum Stigi.Syntax.Parse.Outcome:
  case Parsed(root: Stigi.Syntax.NodeAt, nodes: U64, tokens: U64)
  case ParseRejected(fault: Stigi.Syntax.Parse.Fault, token: U64)
  case LexRejected(offset: U64)
