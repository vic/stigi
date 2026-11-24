// prompt: allocate the reused per-unit tables of the parser and the printer in one region.
pub def Stigi.Syntax.allocate {region: Stigi.Region} () <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Allocation =
  val Filled(source) = Array.tryFill[U8]{region}(Stigi.Limit.maxUnitBytes, 0) else
    case FillFailed => AllocationFailed
  val Filled(tokens) = Array.tryFill[Stigi.Lex.Token]{region}(Stigi.Limit.maxUnitBytes, Token(TokEnd, 0, 0, 0, 0)) else
    case FillFailed => AllocationFailed
  val Filled(nodes) = Array.tryFill[Stigi.Syntax.Node]{region}(Stigi.Limit.maxUnitNodes, Absent(0)) else
    case FillFailed => AllocationFailed
  val Filled(lists) = Array.tryFill[Stigi.Syntax.NodeAt]{region}(Stigi.Limit.maxUnitNodes, 0) else
    case FillFailed => AllocationFailed
  val Filled(pending) = Array.tryFill[Stigi.Syntax.NodeAt]{region}(Stigi.Limit.maxUnitNodes, 0) else
    case FillFailed => AllocationFailed
  val Filled(output) = Array.tryFill[U8]{region}(Stigi.Limit.maxUnitBytes, 0) else
    case FillFailed => AllocationFailed
  Allocated(Tables(source, tokens, nodes, lists, pending, output))
