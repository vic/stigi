// prompt: POSIX handler implementing Stigi.Clock via clock_gettime and getrusage.
pub handler Stigi.Posix.Clock() for Stigi.Clock:
  def monotonicStamp {region: Stigi.Region, Stigi.Mut} (): Stigi.Clock.Stamp =
    val Filled(specBuf) = Array.tryFill[U8]{region}(16, 0) else
      case FillFailed => StampFailed
    val status: I32 = clock_gettime(1, specBuf)
    if status == 0 then
      val secWord: U64 = Stigi.Raw.readLittleLong(16, specBuf, 0)
      val nanoWord: U64 = Stigi.Raw.readLittleLong(16, specBuf, 8)
      if nanoWord < 1000000000 then StampValid(secWord, nanoWord) else StampFailed
    else StampFailed
  def processUsage {region: Stigi.Region, Stigi.Mut} (): Stigi.Clock.Usage =
    val Filled(usageBuf) = Array.tryFill[U8]{region}(144, 0) else
      case FillFailed => UsageFailed
    val status: I32 = getrusage(0, usageBuf)
    if status == 0 then
      val rssWord: U64 = Stigi.Raw.readLittleLong(144, usageBuf, 32)
      val minWord: U64 = Stigi.Raw.readLittleLong(144, usageBuf, 64)
      val majWord: U64 = Stigi.Raw.readLittleLong(144, usageBuf, 72)
      UsageValid(rssWord, minWord, majWord)
    else UsageFailed
