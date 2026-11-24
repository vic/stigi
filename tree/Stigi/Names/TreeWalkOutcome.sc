// prompt: the outcome of walking a tree from a line position: the grown edges, or the unit index and name size of the unit that stopped the walk and why.
enum Stigi.Names.TreeWalkOutcome:
  case TreeWalked(edges: Stigi.Names.Edges)
  case TreeWalkStopped(entry: U64, nameSize: U64, why: Stigi.Names.TreeFailure) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes)>
