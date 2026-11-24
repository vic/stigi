// prompt: return true when an exact wide integer equals zero.
def Stigi.Types.Linear.isZeroWide(value: Stigi.Exact.IntWide): Bool =
  value match
    case WideInt(lowWord, highWord) => lowWord == 0 && highWord == 0
