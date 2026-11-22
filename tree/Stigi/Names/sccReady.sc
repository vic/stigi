// prompt: check whether every outgoing dependency of a candidate SCC is already assigned a component id.
def Stigi.Names.sccReady(nodeCount: U64, edgeCount: U64, edgeFrom: Array[U64](edgeCount), edgeTo: Array[U64](edgeCount), matrixCap: U64, reachMatrix: Array[U8](matrixCap), compOf: Array[U64](nodeCount), seedNode: U64) <below(0, nodeCount), below(nodeCount, 256), below(edgeCount, 65536), below(seedNode, nodeCount), below(matrixCap, 65536)>: Bool =
  var ready: Bool = true
  var edgeIdx: U64 = 0
  while edgeIdx < edgeCount && ready do
    @<lessOrEqual(edgeIdx, edgeCount), decreases(edgeCount - edgeIdx)>
    val src: U64 = edgeFrom(edgeIdx)
    val dst: U64 = edgeTo(edgeIdx)
    if src < nodeCount && dst < nodeCount then
      if Stigi.Names.mutuallyReaches(nodeCount, matrixCap, reachMatrix, seedNode, src) then
        val insideScc: Bool = Stigi.Names.mutuallyReaches(nodeCount, matrixCap, reachMatrix, seedNode, dst)
        if !insideScc && compOf(dst) == nodeCount then
          ready = false
        else
          ()
      else
        ()
    else
      ()
    edgeIdx = edgeIdx + 1
  ready
