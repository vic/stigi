// prompt: check whether two nodes mutually reach each other in the reachability matrix.
def Stigi.Names.mutuallyReaches(nodeCount: U64, matrixCap: U64, reachMatrix: Array[U8](matrixCap), leftNode: U64, rightNode: U64) <below(0, nodeCount), below(nodeCount, 256), below(leftNode, nodeCount), below(rightNode, nodeCount), below(matrixCap, 65536)>: Bool =
  val fwdPos: U64 = U64.wrappingAdd(U64.wrappingMultiply(leftNode, nodeCount), rightNode)
  val revPos: U64 = U64.wrappingAdd(U64.wrappingMultiply(rightNode, nodeCount), leftNode)
  if fwdPos < matrixCap && revPos < matrixCap then
    reachMatrix(fwdPos) != 0 && reachMatrix(revPos) != 0
  else
    false
