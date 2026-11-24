// prompt: collect and write typeRange, typeWidth, lengthTerm, linearForm, termOffset, and sameLength fact files into an existing output directory, stopping before any write on failure.
def Stigi.Store.writeTypesFacts {region: Stigi.Region, Stigi.Mut} (target: Stigi.Store.TypesTarget, image: Stigi.Names.Image, synTables: Stigi.Syntax.Tables): Stigi.Store.TypesWriteOutcome =
  val Filled(rangeBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => TypesWriteFailed(FactsOutOfMemory)
  val Filled(widthBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => TypesWriteFailed(FactsOutOfMemory)
  val TypeCursors(rangeLen, widthLen) = Stigi.Store.collectTypeTables{region}(rangeBuf, widthBuf, 0, 0)
  target match
    case TypesTarget(dirCap, dirBuf, _, _, outStart, outEnd) =>
      @<lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, dirCap)>
      val LengthsCollected(LengthLanes(lengthLane, linearLane, offsetLane, sameLane, rowCount)) = Stigi.Store.collectAllLengths{region}(target, image, synTables, 262144) else
        case LengthsStopped(failure) => TypesWriteFailed(failure)
      val FactsWritten = Stigi.Store.writeFactsFile{region}(dirCap, dirBuf, outStart, outEnd, TypeRangeFile, 4096, rangeBuf, rangeLen) else
        case FactsFailed(failure) => TypesWriteFailed(failure)
      val FactsWritten = Stigi.Store.writeFactsFile{region}(dirCap, dirBuf, outStart, outEnd, TypeWidthFile, 4096, widthBuf, widthLen) else
        case FactsFailed(failure) => TypesWriteFailed(failure)
      val FactsWritten = Stigi.Store.writeLengthLaneFile{region}(target, LengthTermFile, lengthLane) else
        case FactsFailed(failure) => TypesWriteFailed(failure)
      val FactsWritten = Stigi.Store.writeLengthLaneFile{region}(target, LinearFormFile, linearLane) else
        case FactsFailed(failure) => TypesWriteFailed(failure)
      val FactsWritten = Stigi.Store.writeLengthLaneFile{region}(target, TermOffsetFile, offsetLane) else
        case FactsFailed(failure) => TypesWriteFailed(failure)
      val FactsWritten = Stigi.Store.writeLengthLaneFile{region}(target, SameLengthFile, sameLane) else
        case FactsFailed(failure) => TypesWriteFailed(failure)
      if rowCount <= Stigi.Limit.maxEmitBytes then TypesWritten(10 + rowCount) else TypesWriteFailed(FactsOutOfMemory)
