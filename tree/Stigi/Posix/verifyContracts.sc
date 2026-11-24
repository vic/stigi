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
    val pathLen: U64 = Stigi.Array.copy(23, tagBytes, 0, 23, 4096, pathBuf, 0)
    if pathLen < 4096 then
      @<below(pathLen, 4096)>
      val Opened(writeDesc) = Stigi.Posix.openPath(pathLen, pathBuf, Stigi.Posix.writeCreateTruncate) else
        case OpenFailed => RunContractError
      if Stigi.Posix.writeContractFixture(writeDesc) is RunSuccess then
        val Opened(readDesc) = Stigi.Posix.openPath(pathLen, pathBuf, Stigi.Posix.readOnly) else
          case OpenFailed => RunContractError
        val readOutcome: Stigi.RunOutcome = Stigi.Posix.readContractFixture{region}(readDesc)
        if Stigi.Posix.removePath(pathLen, pathBuf) && readOutcome is RunSuccess then RunSuccess else RunContractError
      else if Stigi.Posix.removePath(pathLen, pathBuf) then RunFileError else RunContractError
    else RunContractError
