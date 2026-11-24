// prompt: the outcome of parsing and printing one unit text: the printed text equals the source, differs from it, or the unit failed to load, lex, parse or print.
pub enum Stigi.Syntax.RoundTrip:
  case TextMatched(bytes: U64, tokens: U64, nodes: U64)
  case TextMismatched(printed: U64)
  case UnitLexFailed(offset: U64)
  case UnitParseFailed(fault: Stigi.Syntax.Parse.Fault, offset: U64)
  case UnitPrintFailed
  case UnitUnreadable
