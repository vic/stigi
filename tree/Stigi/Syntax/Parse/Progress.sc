// prompt: the position of a parse in its unit: the next token and the next free entries of the node, list and pending tables.
pub enum Stigi.Syntax.Parse.Progress:
  case Progress(token: U64, nodes: U64, lists: U64, pending: U64)
