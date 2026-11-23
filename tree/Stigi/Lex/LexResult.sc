// prompt: closed result of lexing a source buffer into a typed token array.
pub enum Stigi.Lex.LexResult:
  case LexDone(count: U64)
  case LexOverflow
  case LexSyntaxError(offset: U64, lineNum: U64)
