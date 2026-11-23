// prompt: compute strongly connected components in linear time using iterative Tarjan DFS with no 256-node cap.
def Stigi.Names.computeComponents {region: Stigi.Region, Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, edgeHead: Array[U64](nodeCap), edgeNext: Array[U64](edgeCap), edgeTarget: Array[U64](edgeCap), compOut: Array[U64](nodeCap)) <lessOrEqual(nodeCount, nodeCap)>: Stigi.Names.TarjanResult =
  val Filled(discTime) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => TarjanOutOfMemory
  val Filled(lowLink) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => TarjanOutOfMemory
  val Filled(onStack) = Array.tryFill[Bool]{region}(nodeCap, false) else
    case FillFailed => TarjanOutOfMemory
  val Filled(sccStack) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => TarjanOutOfMemory
  val Filled(dfsNode) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => TarjanOutOfMemory
  val Filled(dfsEdge) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => TarjanOutOfMemory
  var clockTime: U64 = 0
  var compCount: U64 = 0
  var sccTop: U64 = 0
  var startNode: U64 = 0
  while startNode < nodeCount do
    @<lessOrEqual(startNode, nodeCount), lessOrEqual(clockTime, nodeCount), lessOrEqual(compCount, nodeCount), lessOrEqual(sccTop, nodeCap)>
    if discTime(startNode) == 0 then
      if clockTime < nodeCount then
        clockTime = clockTime + 1
        discTime(startNode) := clockTime
        lowLink(startNode) := clockTime
      else ()
      if sccTop < nodeCap then
        sccStack(sccTop) := startNode
        sccTop = sccTop + 1
        onStack(startNode) := true
      else ()
      dfsNode(0) := startNode
      dfsEdge(0) := edgeHead(startNode)
      var dfsTop: U64 = 0
      dfsTop = dfsTop + 1
      var stepGuard: U64 = 0
      val maxSteps: U64 = nodeCount + edgeCap + nodeCount + 4
      while dfsTop > 0 && stepGuard < maxSteps do
        @<lessOrEqual(dfsTop, nodeCap), lessOrEqual(sccTop, nodeCap), lessOrEqual(clockTime, nodeCount), lessOrEqual(compCount, nodeCount), lessOrEqual(stepGuard, maxSteps)>
        val frameIdx: U64 = dfsTop - 1
        val curNode: U64 = dfsNode(frameIdx)
        val curEdge: U64 = dfsEdge(frameIdx)
        if curEdge < edgeCap then
          dfsEdge(frameIdx) := edgeNext(curEdge)
          val nextNode: U64 = edgeTarget(curEdge)
          if nextNode < nodeCount && curNode < nodeCount then
            if discTime(nextNode) == 0 then
              if clockTime < nodeCount then
                clockTime = clockTime + 1
                discTime(nextNode) := clockTime
                lowLink(nextNode) := clockTime
              else ()
              if sccTop < nodeCap then
                sccStack(sccTop) := nextNode
                sccTop = sccTop + 1
                onStack(nextNode) := true
              else ()
              if dfsTop < nodeCap then
                dfsNode(dfsTop) := nextNode
                dfsEdge(dfsTop) := edgeHead(nextNode)
                dfsTop = dfsTop + 1
              else ()
            else if onStack(nextNode) then
              if discTime(nextNode) < lowLink(curNode) then
                lowLink(curNode) := discTime(nextNode)
              else ()
            else ()
          else ()
        else
          dfsTop = frameIdx
          if curNode < nodeCount then
            if dfsTop > 0 then
              val parIdx: U64 = dfsTop - 1
              val parNode: U64 = dfsNode(parIdx)
              if parNode < nodeCount && lowLink(curNode) < lowLink(parNode) then
                lowLink(parNode) := lowLink(curNode)
              else ()
            else ()
            if lowLink(curNode) == discTime(curNode) then
              var popMore: Bool = true
              while popMore && sccTop > 0 do
                @<lessOrEqual(sccTop, nodeCap)>
                val popIdx: U64 = sccTop - 1
                val member: U64 = sccStack(popIdx)
                sccTop = popIdx
                if member < nodeCount then
                  onStack(member) := false
                  compOut(member) := compCount
                  if member == curNode then popMore = false else ()
                else
                  popMore = false
              if compCount < nodeCount then compCount = compCount + 1 else ()
            else ()
          else ()
        stepGuard = stepGuard + 1
    else ()
    startNode = startNode + 1
  TarjanDone(compCount)
