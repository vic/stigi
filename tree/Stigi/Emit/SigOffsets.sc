// prompt: record token positions of a function or top-level value signature and body start.
pub enum Stigi.Emit.SigOffsets:
  case DefSignature(paramListStart: U64, afterParams: U64, returnTok: U64, hasReturnTok: Bool, bodyTok: U64)
