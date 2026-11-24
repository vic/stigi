// prompt: verify that 140-step IndexRead left spines print iteratively without TooDeep.
def Stigi.Syntax.Print.Tests.checkIndexSpine {region: Stigi.Region, Stigi.Mut} (depth: U64) <lessOrEqual(depth, Stigi.Limit.maxUnitNodes)>: Bool =
  val Allocated(tables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => false
  val Tables(source, _, nodes, _, _, output) = tables
  val prefix: Array[U8](4) = "arr0"
  Stigi.Array.copy[U8](4, prefix, 0, 4, Stigi.Limit.maxUnitBytes, source, 0)
  if 1000 < depth || Stigi.Limit.maxUnitNodes <= depth + 2 then false else
    nodes(0) := Ref(0, Text(0, 3))
    nodes(1) := Literal(1, IntLiteral, Text(3, 4))
    var step: U64 = 0
    var cursor: U64 = 0
    while step < depth do
      @<lessOrEqual(step, depth), below(cursor, Stigi.Limit.maxUnitNodes)>
      val slot: U64 = step + 2
      if cursor < slot && 1 < slot then
        nodes(slot) := IndexRead(slot, cursor, 1)
        cursor = slot
      else ()
      step = step + 1
    Stigi.Syntax.Print.printNode(tables, cursor, Pen(0), 0, Stigi.Limit.maxParseDepth) match
      case PenOverflow | PenFailed(_) => false
      case Pen(printedLen) => printedLen == 3 + depth + depth + depth && output(0) == 'a' && output(3) == '(' && output(4) == '0' && output(5) == ')'
