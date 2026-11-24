// prompt: take nodes from the breadth-first queue head and enqueue their unreached targets until the queue is empty.
def Stigi.Names.drainQueue {Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, heads: Array[Stigi.Names.EdgeLink](nodeCap), edges: Array[Stigi.Names.Edge](edgeCap), reached: Array[Bool](nodeCap), queue: Array[U64](nodeCap), head: U64, tail: U64, remaining: U64) <lessOrEqual(nodeCount, nodeCap), lessOrEqual(tail, nodeCount), decreases(remaining)>: U64 <lessOrEqual(result, nodeCount)> =
  if remaining < 1 || tail <= head then tail else
    val node: U64 = queue(head)
    val grown: U64 = if node < nodeCap then Stigi.Names.visitEdges(nodeCap, nodeCount, edgeCap, edges, reached, queue, heads(node), tail, edgeCap) else tail
    Stigi.Names.drainQueue(nodeCap, nodeCount, edgeCap, heads, edges, reached, queue, head + 1, grown, remaining - 1)
