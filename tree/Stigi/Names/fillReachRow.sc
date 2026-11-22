// prompt: run forward BFS from one start node and mark every reachable node in the flat matrix.
def Stigi.Names.fillReachRow {Stigi.Mut} (nodeCount: U64, edgeCount: U64, edgeFrom: Array[U64](edgeCount), edgeTo: Array[U64](edgeCount), startNode: U64, matrixCap: U64, reachMatrix: Array[U8](matrixCap), queue: Array[U64](nodeCount), seen: Array[U8](nodeCount)) <below(0, nodeCount), below(nodeCount, 256), below(edgeCount, 65536), below(startNode, nodeCount), below(matrixCap, 65536)>: Unit =
  var clearIdx: U64 = 0
  while clearIdx < nodeCount do
    @<lessOrEqual(clearIdx, nodeCount), decreases(nodeCount - clearIdx)>
    seen(clearIdx) := 0
    clearIdx = clearIdx + 1
  seen(startNode) := 1
  queue(0) := startNode
  var head: U64 = 0
  var tail: U64 = 0
  tail = tail + 1
  while head < tail do
    @<lessOrEqual(head, tail), lessOrEqual(tail, nodeCount), decreases(nodeCount - head)>
    val current: U64 = queue(head)
    head = head + 1
    var edgeIdx: U64 = 0
    while edgeIdx < edgeCount do
      @<lessOrEqual(edgeIdx, edgeCount), lessOrEqual(head, tail), lessOrEqual(tail, nodeCount), decreases(edgeCount - edgeIdx)>
      if edgeFrom(edgeIdx) == current then
        val target: U64 = edgeTo(edgeIdx)
        if target < nodeCount && seen(target) == 0 && tail < nodeCount then
          seen(target) := 1
          queue(tail) := target
          tail = tail + 1
        else
          ()
      else
        ()
      edgeIdx = edgeIdx + 1
  val rowBase: U64 = U64.wrappingMultiply(startNode, nodeCount)
  var colIdx: U64 = 0
  while colIdx < nodeCount do
    @<lessOrEqual(colIdx, nodeCount), decreases(nodeCount - colIdx)>
    val cellPos: U64 = U64.wrappingAdd(rowBase, colIdx)
    if cellPos < matrixCap then
      reachMatrix(cellPos) := seen(colIdx)
    else
      ()
    colIdx = colIdx + 1
