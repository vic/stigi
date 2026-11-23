// prompt: decoded monotonic timestamp returned by Stigi.Clock.
pub enum Stigi.Clock.Stamp:
  case StampValid(seconds: U64, nanos: U64) <below(nanos, 1000000000)>
  case StampFailed
