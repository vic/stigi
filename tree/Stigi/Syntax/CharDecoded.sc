// prompt: the byte value of one character literal text, or a marker that the text is malformed.
pub enum Stigi.Syntax.CharDecoded:
  case CharDecoded(value: U64)
  case CharMalformed
