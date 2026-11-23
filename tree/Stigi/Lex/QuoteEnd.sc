// prompt: the span of a quoted literal from its opening quote to after its closing quote, or no closing quote.
enum Stigi.Lex.QuoteEnd:
  case Closed(startPos: U64, endPos: U64) <below(startPos, endPos)>
  case Unclosed
