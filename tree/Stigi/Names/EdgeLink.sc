// prompt: the link from a node or an edge to the next edge of its list: none, or an edge index.
enum Stigi.Names.EdgeLink:
  case LastEdge
  case EdgeAt(edgeIdx: U64)
