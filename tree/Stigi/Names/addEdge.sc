// prompt: record one dependency edge of a source unit, dropping targets outside the image, repeats of the source and edges past the limit.
def Stigi.Names.addEdge {Stigi.Mut} (edges: Stigi.Names.Edges, from: U64, to: U64) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Edges =
  val Edges(sources, targets, count, stamps) = edges
  if to < Stigi.Limit.maxUnits && count < Stigi.Names.maxEdges then
    if stamps(to) == from + 1 then edges else
      stamps(to) := from + 1
      sources(count) := from
      targets(count) := to
      Edges(sources, targets, count + 1, stamps)
  else edges
