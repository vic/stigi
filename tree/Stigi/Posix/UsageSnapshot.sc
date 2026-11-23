// prompt: decoded memory and page-fault counters from POSIX getrusage.
pub enum Stigi.Posix.UsageSnapshot:
  case UsageData(maxRssKilobytes: U64, minorFaults: U64, majorFaults: U64)
  case UsageFailed
