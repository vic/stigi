// prompt: compute strongly connected components in dependency-first topological order over a flat reachability matrix.
pub def Stigi.Names.computeComponents {region: Stigi.Region, Stigi.Mut} (nodeCount: U64, edgeCount: U64, edgeFrom: Array[U64](edgeCount), edgeTo: Array[U64](edgeCount), compOf: Array[U64](nodeCount)) <below(0, nodeCount), below(nodeCount, 256), below(edgeCount, 65536)>: U64 <lessOrEqual(result, nodeCount)> =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val matrixCap: U64 = nodeCount * nodeCount
  if matrixCap >= 65536 then
    0
  else
    Array.tryFill{region}(matrixCap, zeroByte) match
      case FillFailed => 0
      case Filled(reachMatrix) =>
        Array.tryFill{region}(nodeCount, zeroWord) match
          case FillFailed => 0
          case Filled(queue) =>
            Array.tryFill{region}(nodeCount, zeroByte) match
              case FillFailed => 0
              case Filled(seen) =>
                var initIdx: U64 = 0
                while initIdx < nodeCount do
                  @<lessOrEqual(initIdx, nodeCount), decreases(nodeCount - initIdx)>
                  compOf(initIdx) := nodeCount
                  Stigi.Names.fillReachRow(nodeCount, edgeCount, edgeFrom, edgeTo, initIdx, matrixCap, reachMatrix, queue, seen)
                  initIdx = initIdx + 1
                var compCount: U64 = 0
                var roundIdx: U64 = 0
                while roundIdx < nodeCount do
                  @<lessOrEqual(roundIdx, nodeCount), lessOrEqual(compCount, roundIdx), decreases(nodeCount - roundIdx)>
                  var chosen: U64 = nodeCount
                  var cand: U64 = 0
                  while cand < nodeCount && chosen == nodeCount do
                    @<lessOrEqual(cand, nodeCount), lessOrEqual(chosen, nodeCount), decreases(nodeCount - cand)>
                    if compOf(cand) == nodeCount && Stigi.Names.sccReady(nodeCount, edgeCount, edgeFrom, edgeTo, matrixCap, reachMatrix, compOf, cand) then
                      chosen = cand
                    else
                      ()
                    cand = cand + 1
                  if chosen < nodeCount then
                    var member: U64 = 0
                    while member < nodeCount do
                      @<lessOrEqual(member, nodeCount), decreases(nodeCount - member)>
                      if Stigi.Names.mutuallyReaches(nodeCount, matrixCap, reachMatrix, chosen, member) then
                        compOf(member) := compCount
                      else
                        ()
                      member = member + 1
                    compCount = compCount + 1
                  else
                    ()
                  roundIdx = roundIdx + 1
                compCount
