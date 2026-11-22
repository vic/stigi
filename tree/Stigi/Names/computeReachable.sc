// prompt: compute root reachability over a directed unit dependency graph for tree shaking.
pub def Stigi.Names.computeReachable {region: Stigi.Region, Stigi.Mut} (nodeCount: U64, edgeCount: U64, edgeFrom: Array[U64](edgeCount), edgeTo: Array[U64](edgeCount), isRoot: Array[U8](nodeCount), reachable: Array[U8](nodeCount)) <below(0, nodeCount), below(nodeCount, 4096), below(edgeCount, 65536)>: U64 <lessOrEqual(result, nodeCount)> =
  val zeroWord: U64 = 0
  Array.tryFill{region}(nodeCount, zeroWord) match
    case FillFailed => 0
    case Filled(queue) =>
      var head: U64 = 0
      var tail: U64 = 0
      var initIdx: U64 = 0
      while initIdx < nodeCount do
        @<lessOrEqual(initIdx, nodeCount), lessOrEqual(tail, initIdx), decreases(nodeCount - initIdx)>
        if isRoot(initIdx) != 0 then
          reachable(initIdx) := 1
          queue(tail) := initIdx
          tail = tail + 1
        else
          reachable(initIdx) := 0
        initIdx = initIdx + 1
      while head < tail do
        @<lessOrEqual(head, tail), lessOrEqual(tail, nodeCount), decreases(nodeCount - head)>
        val current: U64 = queue(head)
        head = head + 1
        var edgeIdx: U64 = 0
        while edgeIdx < edgeCount do
          @<lessOrEqual(edgeIdx, edgeCount), lessOrEqual(head, tail), lessOrEqual(tail, nodeCount), decreases(edgeCount - edgeIdx)>
          if edgeFrom(edgeIdx) == current then
            val target: U64 = edgeTo(edgeIdx)
            if target < nodeCount && reachable(target) == 0 && tail < nodeCount then
              reachable(target) := 1
              queue(tail) := target
              tail = tail + 1
            else
              ()
          else
            ()
          edgeIdx = edgeIdx + 1
      tail
