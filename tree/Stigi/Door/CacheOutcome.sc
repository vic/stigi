// prompt: result of probing the persistent BLAKE3-keyed verdict cache.
pub enum Stigi.Door.CacheOutcome:
  case CacheHit(passed: Bool)
  case CacheMiss
