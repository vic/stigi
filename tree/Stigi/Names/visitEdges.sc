// prompt: enqueue every unreached target along an edge list.
def Stigi.Names.visitEdges {Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, edges: Array[Stigi.Names.Edge](edgeCap), reached: Array[Bool](nodeCap), queue: Array[U64](nodeCap), link: Stigi.Names.EdgeLink, tail: U64, remaining: U64) <lessOrEqual(nodeCount, nodeCap), lessOrEqual(tail, nodeCount), decreases(remaining)>: U64 <lessOrEqual(result, nodeCount)> =
  if remaining < 1 then tail else
    link match
      case LastEdge => tail
      case EdgeAt(edgeIdx) =>
        if edgeCap <= edgeIdx then tail else
          val Edge(target, next) = edges(edgeIdx)
          val grown: U64 = if target < nodeCount && !reached(target) && tail < nodeCount then Stigi.Names.enqueue(nodeCap, nodeCount, reached, queue, tail, target) else tail
          Stigi.Names.visitEdges(nodeCap, nodeCount, edgeCap, edges, reached, queue, next, grown, remaining - 1)
