// prompt: byte span of an unwrapped frame inside a transport payload.
pub enum Stigi.Server.EnvelopeOutcome:
  case EnvelopeSpan(startOffset: U64, endOffset: U64)
  case EnvelopeInvalid
