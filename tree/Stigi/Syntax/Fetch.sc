// prompt: the outcome of reading one slot of a node table: the node, or a slot that does not hold its own index.
enum Stigi.Syntax.Fetch:
  case Fetched(node: Stigi.Syntax.Node)
  case MalformedTable(slot: U64)
