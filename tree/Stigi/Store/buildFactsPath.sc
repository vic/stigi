// prompt: build the null terminated path of a facts file inside an output directory that already exists, giving its length, or an overflow when it does not fit.
def Stigi.Store.buildFactsPath {Stigi.Mut} (outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), outStart: U64, outEnd: U64, file: Stigi.Store.FactsFile, path: Array[U8](Stigi.Limit.maxPathBytes)) <lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, outCap)>: Stigi.Store.FactsPath =
  val dirLen: U64 = Stigi.Array.copy[U8](outCap, outBuf, outStart, outEnd, Stigi.Limit.maxPathBytes, path, 0)
  val slashed: U64 = Stigi.Array.copy[U8](1, "/", 0, 1, Stigi.Limit.maxPathBytes, path, dirLen)
  val total: U64 = Stigi.Store.putFileName(Stigi.Limit.maxPathBytes, path, slashed, file)
  if total < Stigi.Limit.maxPathBytes then
    path(total) := '\0'
    PathBuilt(total)
  else PathOverflow
