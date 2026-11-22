// prompt: result of scanning one token from a source byte array.
pub enum Stigi.Lex.ScanOutcome:
  case ScanToken(kind: Stigi.Lex.TokenKind, startOffset: U64, endOffset: U64)
  case ScanError(offset: U64, reasonCode: U64)
