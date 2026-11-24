// prompt: mark a node reached and append it to the breadth-first queue, giving the new queue tail.
def Stigi.Names.enqueue {Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, reached: Array[Bool](nodeCap), queue: Array[U64](nodeCap), tail: U64, node: U64) <lessOrEqual(nodeCount, nodeCap), below(tail, nodeCount), below(node, nodeCap)>: U64 <lessOrEqual(result, nodeCount)> =
  reached(node) := true
  queue(tail) := node
  tail + 1
