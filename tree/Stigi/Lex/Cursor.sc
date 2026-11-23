// prompt: scanner position, line, line start and written token count between two lexical items.
enum Stigi.Lex.Cursor:
  case Cursor(pos: U64, lineNum: U64, lineStart: U64, tokCount: U64)
