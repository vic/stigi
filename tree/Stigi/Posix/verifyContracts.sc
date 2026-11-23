// prompt: execute live POSIX contract checks over Stigi.File and Stigi.Clock.
pub def Stigi.Posix.verifyContracts {region: Stigi.Region, clock: Stigi.Clock, Stigi.Mut} (): Stigi.RunOutcome =
  val StampValid(_, _) = clock.monotonicStamp{region}() else
    case StampFailed => RunContractError
  val UsageValid(rssWord, _, _) = clock.processUsage{region}() else
    case UsageFailed => RunContractError
  if rssWord == 0 then RunContractError else
    val Filled(pathBuf) = Array.tryFill[U8]{region}(4096, 0) else
      case FillFailed => RunOutOfMemory
    val tagBytes: Array[U8](23) = "/tmp/stigi-v2-check.tmp"
    var idx: U64 = 0
    while idx < 23 do
      @<lessOrEqual(idx, 23)>
      pathBuf(idx) := tagBytes(idx)
      idx = idx + 1
    val Opened(writeDesc) = Stigi.Posix.openWrite(23, pathBuf) else
      case OpenFailed => RunContractError
    val wroteValid: Bool = Stigi.Posix.writeContractFixture(writeDesc)
    if !wroteValid then
      val removedErr: Bool = Stigi.Posix.removePath(23, pathBuf)
      if removedErr then RunFileError else RunContractError
    else
      val Opened(readDesc) = Stigi.Posix.openRead(23, pathBuf) else
        case OpenFailed => RunContractError
      val readOutcome: Stigi.RunOutcome = Stigi.Posix.readContractFixture{region}(readDesc)
      val unlinked: Bool = Stigi.Posix.removePath(23, pathBuf)
      (readOutcome, unlinked) match
        case (RunSuccess, true) => RunSuccess
        case (_, _) => RunContractError
