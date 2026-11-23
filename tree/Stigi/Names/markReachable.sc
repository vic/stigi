// prompt: compute reachable units in linear time via breadth-first traversal from roots for tree-shaking.
def Stigi.Names.markReachable {region: Stigi.Region, Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, edgeHead: Array[U64](nodeCap), edgeNext: Array[U64](edgeCap), edgeTarget: Array[U64](edgeCap), isRoot: Array[Bool](nodeCap), reached: Array[Bool](nodeCap)) <lessOrEqual(nodeCount, nodeCap)>: U64 <lessOrEqual(result, nodeCount)> =
  val Filled(queue) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => 0
  var queueHead: U64 = 0
  var queueTail: U64 = 0
  var idx: U64 = 0
  while idx < nodeCount do
    @<lessOrEqual(idx, nodeCount), lessOrEqual(queueTail, nodeCount)>
    if isRoot(idx) then
      reached(idx) := true
      if queueTail < nodeCount then
        queue(queueTail) := idx
        queueTail = queueTail + 1
      else ()
    else
      reached(idx) := false
    idx = idx + 1
  while queueHead < queueTail do
    @<lessOrEqual(queueHead, queueTail), lessOrEqual(queueTail, nodeCount)>
    val curNode: U64 = queue(queueHead)
    queueHead = queueHead + 1
    if curNode < nodeCount then
      var edgeIdx: U64 = edgeHead(curNode)
      var edgeSteps: U64 = 0
      while edgeIdx < edgeCap && edgeSteps < edgeCap do
        @<lessOrEqual(edgeSteps, edgeCap), lessOrEqual(queueHead, queueTail), lessOrEqual(queueTail, nodeCount)>
        val target: U64 = edgeTarget(edgeIdx)
        if target < nodeCount && !reached(target) then
          reached(target) := true
          if queueTail < nodeCount then
            queue(queueTail) := target
            queueTail = queueTail + 1
          else ()
        else ()
        edgeIdx = edgeNext(edgeIdx)
        edgeSteps = edgeSteps + 1
    else ()
  queueTail
