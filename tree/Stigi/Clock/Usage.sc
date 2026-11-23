// prompt: decoded process resource usage metrics returned by Stigi.Clock.
pub enum Stigi.Clock.Usage:
  case UsageValid(maxRss: U64, minorFaults: U64, majorFaults: U64)
  case UsageFailed
