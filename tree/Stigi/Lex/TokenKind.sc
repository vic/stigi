// prompt: token kinds produced by the lexical scanner.
pub enum Stigi.Lex.TokenKind:
  case TokenEnd
  case TokenNewline
  case TokenIdent
  case TokenUpperIdent
  case TokenInt
  case TokenByte
  case TokenText
  case TokenComment
  case TokenPrompt
  case TokenPunct
