// prompt: one directed edge of a node's edge list: its target node and the link to the next edge.
enum Stigi.Names.Edge:
  case Edge(target: U64, next: Stigi.Names.EdgeLink)
