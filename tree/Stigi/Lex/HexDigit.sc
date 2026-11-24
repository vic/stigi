// prompt: parse-dont-validate hexadecimal nibble outcome carrying proved bound below sixteen.
pub enum Stigi.Lex.HexDigit:
  case HexValue(nibble: U64) <below(nibble, 16)>
  case NotHex
