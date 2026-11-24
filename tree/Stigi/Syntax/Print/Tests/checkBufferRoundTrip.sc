// prompt: check exact parse and round-trip print equality on a synthetic source buffer.
def Stigi.Syntax.Print.Tests.checkBufferRoundTrip {region: Stigi.Region, Stigi.Mut} (bufferCap: U64, sourceBuffer: Array[U8](bufferCap), totalWritten: U64, expectAccept: Bool) <lessOrEqual(totalWritten, bufferCap), lessOrEqual(bufferCap, Stigi.Limit.maxUnitBytes)>: Bool =
  val Allocated(tables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => false
  val Tables(_, _, _, _, _, output) = tables
  Stigi.Syntax.Parse.parseUnit(tables, bufferCap, sourceBuffer, totalWritten) match
    case LexRejected(_) => false
    case ParseRejected(fault, _) => !expectAccept && fault is TooDeep
    case Parsed(root, _, _) =>
      if !expectAccept then false else
        Stigi.Syntax.Print.printNode(tables, root, Pen(0), 0, Stigi.Limit.maxParseDepth) match
          case PenOverflow | PenFailed(_) => false
          case Pen(printedLen) => printedLen == totalWritten && Stigi.Syntax.Print.Tests.bytesEqual(bufferCap, sourceBuffer, Stigi.Limit.maxUnitBytes, output, totalWritten)
