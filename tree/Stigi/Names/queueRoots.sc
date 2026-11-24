// prompt: append every reached root from a node on to the breadth-first queue.
def Stigi.Names.queueRoots {Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, reached: Array[Bool](nodeCap), queue: Array[U64](nodeCap), node: U64, tail: U64, remaining: U64) <lessOrEqual(nodeCount, nodeCap), lessOrEqual(tail, nodeCount), decreases(remaining)>: U64 <lessOrEqual(result, nodeCount)> =
  if remaining < 1 || nodeCount <= node then tail else
    val grown: U64 = if reached(node) && tail < nodeCount then Stigi.Names.enqueue(nodeCap, nodeCount, reached, queue, tail, node) else tail
    Stigi.Names.queueRoots(nodeCap, nodeCount, reached, queue, node + 1, grown, remaining - 1)
