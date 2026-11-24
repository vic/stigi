// prompt: write one collected length lane to its facts file in the output directory.
def Stigi.Store.writeLengthLaneFile {region: Stigi.Region, Stigi.Mut} (target: Stigi.Store.TypesTarget, file: Stigi.Store.FactsFile, lane: Stigi.Store.Lane): Stigi.Store.FactsOutcome =
  target match
    case TypesTarget(dirCap, dirBuf, _, _, outStart, outEnd) =>
      @<lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, dirCap)>
      lane match
        case Lane(size, bytes, end) =>
          Stigi.Store.writeFactsFile{region}(dirCap, dirBuf, outStart, outEnd, file, size, bytes, end)
