// prompt: single-case parsed type reference storing outer type span, bracket element span, and next token.
enum Stigi.Emit.TypeSpan:
  case TypeSpan(typeStart: U64, typeEnd: U64, elemStart: U64, elemEnd: U64, nextTok: U64) <lessOrEqual(typeStart, typeEnd), lessOrEqual(elemStart, elemEnd)>
