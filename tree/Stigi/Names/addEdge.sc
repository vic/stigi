// prompt: record one dependency edge between two units of the image: a repeat of the source keeps the edges as they are and an edge past the limit is the full result.
def Stigi.Names.addEdge {Stigi.Mut} (edges: Stigi.Names.Edges, from: U64, to: U64) <below(from, Stigi.Limit.maxUnits), below(to, Stigi.Limit.maxUnits)>: Stigi.Names.EdgeAdded =
  val Edges(sources, targets, count, stamps) = edges
  if stamps(to) == from + 1 then EdgeKept(edges) else
    if count < Stigi.Names.maxEdges then
      stamps(to) := from + 1
      sources(count) := from
      targets(count) := to
      EdgeKept(Edges(sources, targets, count + 1, stamps))
    else EdgesFull
