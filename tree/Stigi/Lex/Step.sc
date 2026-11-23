// prompt: closed outcome of reading one lexical item at a position of a source buffer.
enum Stigi.Lex.Step:
  case Blank(endPos: U64)
  case Lexed(token: Stigi.Lex.Token, endPos: U64)
  case LineBreak(token: Stigi.Lex.Token, endPos: U64)
  case Malformed
