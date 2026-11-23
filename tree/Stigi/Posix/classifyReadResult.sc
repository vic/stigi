// prompt: classify a raw POSIX read byte count into a typed ReadOutcome bounded by maxBytes.
pub def Stigi.Posix.classifyReadResult(rawCount: I64, maxBytes: U64) <lessOrEqual(0, 1)>: Stigi.Posix.ReadOutcome =
  if rawCount < 0 then
    ReadFailed
  else
    val countVal: U64 = U64.ofI64(rawCount)
    if countVal <= maxBytes then
      ReadBytes(countVal)
    else
      ReadFailed
