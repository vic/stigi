// prompt: tell whether the edges hold one edge from a unit entry to another.
def Stigi.Names.Tests.hasEdge(edges: Stigi.Names.Edges, from: U64, to: U64) <below(from, Stigi.Limit.maxUnits), below(to, Stigi.Limit.maxUnits)>: Bool =
  val Edges(sources, targets, count, _) = edges
  var index: U64 = 0
  var found: Bool = false
  while index < count do
    @<lessOrEqual(index, count)>
    found = found || sources(index) == from && targets(index) == to
    index = index + 1
  found
