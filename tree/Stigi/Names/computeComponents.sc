// prompt: compute strongly connected components in linear time using iterative Tarjan DFS with no 256-node cap.
def Stigi.Names.computeComponents {region: Stigi.Region, Stigi.Mut} (nodeCap: Stigi.UnitCap, nodeCount: U64, edgeCap: Stigi.NodeCap, heads: Array[Stigi.Names.EdgeLink](nodeCap), edges: Array[Stigi.Names.Edge](edgeCap), visits: Array[Stigi.Names.Visit](nodeCap)) <lessOrEqual(nodeCount, nodeCap)>: Stigi.Names.TarjanResult =
  val Filled(frames) = Array.tryFill[Stigi.Names.Frame]{region}(nodeCap, Frame(0, LastEdge)) else
    case FillFailed => TarjanOutOfMemory
  val Filled(sccStack) = Array.tryFill[U64]{region}(nodeCap, 0) else
    case FillFailed => TarjanOutOfMemory
  val Walk(_, comps, _, _) = Stigi.Names.visitFrom(nodeCap, nodeCount, edgeCap, heads, edges, visits, frames, sccStack, 0, Walk(0, 0, 0, 0), nodeCount)
  TarjanDone(comps)
