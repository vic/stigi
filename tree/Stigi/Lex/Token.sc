// prompt: single-case token record packing kind, byte span, line, and column into one array element.
pub enum Stigi.Lex.Token:
  case Token(kind: Stigi.Lex.TokenKind, startPos: U64, endPos: U64, lineNum: U64, colNum: U64) <lessOrEqual(startPos, endPos)>
