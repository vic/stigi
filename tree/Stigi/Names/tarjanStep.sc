// prompt: run the Tarjan depth-first walk one edge or one finished node per step until the frame stack is empty.
def Stigi.Names.tarjanStep {Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, heads: Array[Stigi.Names.EdgeLink](nodeCap), edges: Array[Stigi.Names.Edge](edgeCap), visits: Array[Stigi.Names.Visit](nodeCap), frames: Array[Stigi.Names.Frame](nodeCap), sccStack: Array[U64](nodeCap), walk: Stigi.Names.Walk, remaining: U64) <lessOrEqual(nodeCount, nodeCap), decreases(remaining)>: Stigi.Names.Walk =
  val Walk(clock, comps, sccTop, dfsTop) = walk
  if remaining < 1 || dfsTop < 1 || nodeCap < dfsTop then walk else
    val Frame(node, link) = frames(dfsTop - 1)
    val popped: Stigi.Names.Walk = Walk(clock, comps, sccTop, dfsTop - 1)
    val stepped: Stigi.Names.Walk = if nodeCap <= node then popped else link match
      case LastEdge => Stigi.Names.finishNode(nodeCap, visits, frames, sccStack, node, popped)
      case EdgeAt(edgeIdx) =>
        if edgeCap <= edgeIdx then Stigi.Names.finishNode(nodeCap, visits, frames, sccStack, node, popped) else
          val Edge(target, next) = edges(edgeIdx)
          frames(dfsTop - 1) := Frame(node, next)
          if nodeCount <= target then walk else
            visits(target) match
              case Unseen => Stigi.Names.pushNode(nodeCap, heads, visits, frames, sccStack, target, walk)
              case OnStack(disc, _) =>
                Stigi.Names.lowerLink(nodeCap, visits, node, disc)
                walk
              case Placed(_) => walk
    Stigi.Names.tarjanStep(nodeCap, nodeCount, edgeCap, heads, edges, visits, frames, sccStack, stepped, remaining - 1)
