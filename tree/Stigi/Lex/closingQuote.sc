// prompt: find the closing quote of the literal that opens at a quote position, stepping over escaped bytes.
def Stigi.Lex.closingQuote(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), srcLen: U64, quoteAt: U64, endPos: U64, remaining: U64) <lessOrEqual(srcLen, srcCap), below(quoteAt, srcLen), below(quoteAt, endPos), decreases(remaining)>: Stigi.Lex.QuoteEnd =
  if endPos < srcLen && remaining > 0 then
    src(endPos) match
      case '\\' => Stigi.Lex.closingQuote(srcCap, src, srcLen, quoteAt, endPos + 2, remaining - 1)
      case byte if byte == src(quoteAt) => Closed(quoteAt, endPos + 1)
      case _ => Stigi.Lex.closingQuote(srcCap, src, srcLen, quoteAt, endPos + 1, remaining - 1)
  else Unclosed
