// prompt: represent the decoded byte length of a quoted string literal or a malformed escape.
pub enum Stigi.Syntax.StringLength:
  case DecodedLength(byteCount: U64)
  case MalformedString
