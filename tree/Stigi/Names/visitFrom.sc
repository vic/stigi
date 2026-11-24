// prompt: start a Tarjan walk from every unseen node from a start node on.
def Stigi.Names.visitFrom {Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, heads: Array[Stigi.Names.EdgeLink](nodeCap), edges: Array[Stigi.Names.Edge](edgeCap), visits: Array[Stigi.Names.Visit](nodeCap), frames: Array[Stigi.Names.Frame](nodeCap), sccStack: Array[U64](nodeCap), start: U64, walk: Stigi.Names.Walk, remaining: U64) <lessOrEqual(nodeCount, nodeCap), decreases(remaining)>: Stigi.Names.Walk =
  if remaining < 1 || nodeCount <= start then walk else
    val grown: Stigi.Names.Walk = if visits(start) is Unseen then Stigi.Names.tarjanStep(nodeCap, nodeCount, edgeCap, heads, edges, visits, frames, sccStack, Stigi.Names.pushNode(nodeCap, heads, visits, frames, sccStack, start, walk), nodeCount + edgeCap) else walk
    Stigi.Names.visitFrom(nodeCap, nodeCount, edgeCap, heads, edges, visits, frames, sccStack, start + 1, grown, remaining - 1)
