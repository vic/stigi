// prompt: decode Linux x86-64 struct timespec seconds and nanoseconds at offsets 0 and 8.
pub def Stigi.Posix.decodeClockBuffer(timeBytes: Array[U8](16)) <lessOrEqual(0, 1)>: Stigi.Posix.ClockSnapshot =
  val secs: U64 = Stigi.Posix.readLittleEndianWord(16, timeBytes, 0)
  val nanos: U64 = Stigi.Posix.readLittleEndianWord(16, timeBytes, 8)
  ClockData(secs, nanos)
