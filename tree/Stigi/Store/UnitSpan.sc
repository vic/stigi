// prompt: single-case record holding one loaded unit name, source slice, and token slice.
enum Stigi.Store.UnitSpan:
  case UnitSpan(nameStart: U64, nameEnd: U64, srcStart: U64, srcEnd: U64, tokStart: U64, tokEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(srcStart, srcEnd), lessOrEqual(tokStart, tokEnd)>
