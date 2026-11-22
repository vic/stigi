// prompt: outcome of decoding one minimal unsigned LEB128 integer.
pub enum Stigi.Identity.LebOutcome:
  case LebDecoded(value: U64, nextOffset: U64)
  case LebError(offset: U64, reason: Stigi.Identity.DecodeReason)
