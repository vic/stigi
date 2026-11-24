// prompt: test acyclic type graphs and recursive-type cycle detection over inline and element edges.
def Stigi.Types.Tests.testCycles {region: Stigi.Region, Stigi.Mut} (typeCount: U64) <lessOrEqual(typeCount, 4)>: Bool =
  val padEdge: Stigi.Types.Cycles.TypeEdge = DepEdge(0, 0, InlineEdge)
  Array.tryFill{region}(4, padEdge) match
    case FillFailed => false
    case Filled(edges) =>
      edges(0) := DepEdge(0, 1, InlineEdge)
      edges(1) := DepEdge(1, 2, ElementEdge)
      val chkDag: Bool = Stigi.Types.Cycles.detectCycle{region}(4, typeCount, 4, edges, 2) match
        case AcyclicTypes => true
        case RecursiveCycle(_, _) => false
        case CycleStackFull => false
      edges(1) := DepEdge(1, 0, InlineEdge)
      val chkInline: Bool = Stigi.Types.Cycles.detectCycle{region}(4, typeCount, 4, edges, 2) match
        case RecursiveCycle(_, _) => true
        case AcyclicTypes => false
        case CycleStackFull => false
      edges(0) := DepEdge(0, 1, ElementEdge)
      edges(1) := DepEdge(1, 0, ElementEdge)
      val chkElem: Bool = Stigi.Types.Cycles.detectCycle{region}(4, typeCount, 4, edges, 2) match
        case RecursiveCycle(_, _) => true
        case AcyclicTypes => false
        case CycleStackFull => false
      chkDag && chkInline && chkElem
