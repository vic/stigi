// prompt: the outcome of walking one unit: the edges grown by it, or why the walk failed.
enum Stigi.Names.WalkOutcome:
  case Walked(edges: Stigi.Names.Edges)
  case WalkFailed(why: Stigi.Names.TreeFailure)
