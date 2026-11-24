// prompt: the Tarjan state of a node: not yet seen, on the stack with its discovery time and low link, or placed in a component.
enum Stigi.Names.Visit:
  case Unseen
  case OnStack(disc: U64, low: U64)
  case Placed(comp: U64)
