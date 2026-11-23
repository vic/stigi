// prompt: classify a raw POSIX write byte count into a typed WriteOutcome bounded by maxBytes.
pub def Stigi.Posix.classifyWriteResult(rawCount: I64, maxBytes: U64) <lessOrEqual(0, 1)>: Stigi.Posix.WriteOutcome =
  if rawCount < 0 then
    WriteFailed
  else
    val countVal: U64 = U64.ofI64(rawCount)
    if countVal <= maxBytes then
      WroteBytes(countVal)
    else
      WriteFailed
