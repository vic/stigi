// prompt: write the typeRange and typeWidth fact files into an output directory that must already exist, stopping at the first failure.
def Stigi.Store.writeTypesFacts {region: Stigi.Region, Stigi.Mut} (outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), outStart: U64, outEnd: U64) <lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, outCap)>: Stigi.Store.FactsOutcome =
  val Filled(rangeBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => FactsFailed(FactsOutOfMemory)
  val Filled(widthBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => FactsFailed(FactsOutOfMemory)
  val TypeCursors(rangeLen, widthLen) = Stigi.Store.collectTypeTables{region}(rangeBuf, widthBuf, 0, 0)
  val FactsWritten = Stigi.Store.writeFactsFile{region}(outCap, outBuf, outStart, outEnd, TypeRangeFile, 4096, rangeBuf, rangeLen) else
    case FactsFailed(failure) => FactsFailed(failure)
  Stigi.Store.writeFactsFile{region}(outCap, outBuf, outStart, outEnd, TypeWidthFile, 4096, widthBuf, widthLen)
