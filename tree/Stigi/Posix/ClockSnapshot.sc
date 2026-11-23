// prompt: decoded seconds and nanoseconds from POSIX clock_gettime.
pub enum Stigi.Posix.ClockSnapshot:
  case ClockData(seconds: U64, nanoseconds: U64)
  case ClockFailed
