// prompt: one depth-first frame: the node and the link to its next unscanned edge.
enum Stigi.Names.Frame:
  case Frame(node: U64, link: Stigi.Names.EdgeLink)
