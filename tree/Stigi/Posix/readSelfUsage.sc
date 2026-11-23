// prompt: sample current process memory and fault counters via POSIX getrusage.
def Stigi.Posix.readSelfUsage {region: Stigi.Region, Stigi.Mut} () <lessOrEqual(0, 1)>: Stigi.Posix.UsageSnapshot =
  val zeroByte: U8 = 0
  Array.tryFill{region}(144, zeroByte) match
    case FillFailed => UsageFailed
    case Filled(usageBuf) =>
      if Stigi.Posix.getrusage(0, usageBuf) == 0 then
        Stigi.Posix.decodeUsageBuffer(usageBuf)
      else
        UsageFailed
