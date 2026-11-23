// prompt: single-case split positions locating then and matching else tokens for an if expression.
enum Stigi.Emit.IfSplit:
  case IfParts(thenPos: U64, elsePos: U64) <lessOrEqual(thenPos, elsePos)>
  case IfIncomplete
