// prompt: sample the POSIX monotonic clock into a typed ClockSnapshot.
def Stigi.Posix.readMonotonicClock {region: Stigi.Region, Stigi.Mut} () <lessOrEqual(0, 1)>: Stigi.Posix.ClockSnapshot =
  val zeroByte: U8 = 0
  Array.tryFill{region}(16, zeroByte) match
    case FillFailed => ClockFailed
    case Filled(timeBuf) =>
      if Stigi.Posix.clock_gettime(1, timeBuf) == 0 then
        Stigi.Posix.decodeClockBuffer(timeBuf)
      else
        ClockFailed
