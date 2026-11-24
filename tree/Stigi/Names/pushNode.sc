// prompt: discover a node: give it the next discovery time and push it on the component and frame stacks.
def Stigi.Names.pushNode {Stigi.Mut} (nodeCap: Stigi.UnitCap, heads: Array[Stigi.Names.EdgeLink](nodeCap), visits: Array[Stigi.Names.Visit](nodeCap), frames: Array[Stigi.Names.Frame](nodeCap), sccStack: Array[U64](nodeCap), node: U64, walk: Stigi.Names.Walk) <below(node, nodeCap)>: Stigi.Names.Walk =
  val Walk(clock, comps, sccTop, dfsTop) = walk
  if nodeCap <= clock || nodeCap <= sccTop || nodeCap <= dfsTop then walk else
    visits(node) := OnStack(clock, clock)
    sccStack(sccTop) := node
    frames(dfsTop) := Frame(node, heads(node))
    Walk(clock + 1, comps, sccTop + 1, dfsTop + 1)
