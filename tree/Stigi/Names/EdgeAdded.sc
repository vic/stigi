// prompt: the result of recording one dependency edge: the edges with it kept, or the full edge table that cannot take another edge.
enum Stigi.Names.EdgeAdded:
  case EdgeKept(edges: Stigi.Names.Edges)
  case EdgesFull
