// prompt: compute reachable units in linear time via breadth-first traversal from the roots marked reached, for tree-shaking.
def Stigi.Names.markReachable {region: Stigi.Region, Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, heads: Array[Stigi.Names.EdgeLink](nodeCap), edges: Array[Stigi.Names.Edge](edgeCap), reached: Array[Bool](nodeCap)) <lessOrEqual(nodeCount, nodeCap)>: U64 <lessOrEqual(result, nodeCount)> =
  val Filled(queue) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => 0
  val tail: U64 = Stigi.Names.queueRoots(nodeCap, nodeCount, reached, queue, 0, 0, nodeCount)
  Stigi.Names.drainQueue(nodeCap, nodeCount, edgeCap, heads, edges, reached, queue, 0, tail, nodeCount)
