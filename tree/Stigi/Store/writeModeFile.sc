// prompt: write the single row of the image mode relation to its file of the existing output directory.
def Stigi.Store.writeModeFile {region: Stigi.Region, Stigi.Mut} (outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), outStart: U64, outEnd: U64) <lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, outCap)>: Stigi.Store.FactsOutcome =
  val Filled(modeBuf) = Array.tryFill[U8]{region}(16, 0) else
    case FillFailed => FactsFailed(FactsOutOfMemory)
  Stigi.Store.writeFactsFile{region}(outCap, outBuf, outStart, outEnd, ModeFile, 16, modeBuf, Stigi.Store.putModeRow(16, modeBuf, 0))
