// prompt: classify a raw POSIX openat return code into a typed OpenOutcome without leaking -1.
pub def Stigi.Posix.classifyOpenResult(rawDesc: I32) <lessOrEqual(0, 1)>: Stigi.Posix.OpenOutcome =
  if rawDesc < 0 then
    OpenFailed
  else
    FileOpened(rawDesc)
