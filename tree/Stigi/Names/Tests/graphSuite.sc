// prompt: verify linear-time Tarjan SCC on cycles and reachability BFS plus splitmix64 table probe.
val Stigi.Names.Tests.graphSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Filled(heads) = Array.tryFill[Stigi.Names.EdgeLink]{Stigi.Region}(8, LastEdge) else
      case FillFailed => false
    val Filled(edges) = Array.tryFill[Stigi.Names.Edge]{Stigi.Region}(8, Edge(0, LastEdge)) else
      case FillFailed => false
    val Filled(visits) = Array.tryFill[Stigi.Names.Visit]{Stigi.Region}(8, Unseen) else
      case FillFailed => false
    val Filled(reached) = Array.tryFill[Bool]{Stigi.Region}(8, false) else
      case FillFailed => false
    val Filled(table) = Array.tryFill[Stigi.Names.KeySlot]{Stigi.Region}(8, Vacant) else
      case FillFailed => false
    heads(0) := EdgeAt(0)
    edges(0) := Edge(1, LastEdge)
    heads(1) := EdgeAt(1)
    edges(1) := Edge(0, LastEdge)
    heads(2) := EdgeAt(2)
    edges(2) := Edge(1, LastEdge)
    val TarjanDone(compCount) = Stigi.Names.computeComponents{Stigi.Region}(8, 4, 8, heads, edges, visits) else
      case TarjanOutOfMemory => false
    reached(2) := true
    val reachCount: U64 = Stigi.Names.markReachable{Stigi.Region}(8, 4, 8, heads, edges, reached)
    val targetSlot: U64 = Stigi.Raw.mixSlot(99, 8)
    table(targetSlot) := Keyed(99)
    val probeHit: Bool = Stigi.sameValue(Stigi.Names.probeTable(8, table, 99, targetSlot, 8), Found(targetSlot))
    val probeMiss: Bool = Stigi.Names.probeTable(8, table, 77, Stigi.Raw.mixSlot(77, 8), 8) is Missing
    val sccValid: Bool = compCount == 3 && visits(0) is Placed(0) && visits(1) is Placed(0) && visits(2) is Placed(1) && visits(3) is Placed(2)
    sccValid && reachCount == 3 && reached(0) && reached(1) && reached(2) && !reached(3) && probeHit && probeMiss
  Stigi.Test.expect(passed, true, "Tarjan SCC, BFS reachability, and hash probe must agree")
