// prompt: check whether an expression range has a depth-zero logical-or operator without a top-level if.
def Stigi.Emit.needsTopOrWrap(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, exprStart: U64, exprEnd: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprStart, exprEnd), lessOrEqual(exprEnd, tokCount), below(srcCap, 0x100000000)>: Bool =
  var scanIdx: U64 = exprStart
  var scanDepth: U64 = 0
  var foundTopOr: Bool = false
  var foundTopIf: Bool = false
  while scanIdx < exprEnd do
    @<lessOrEqual(exprStart, scanIdx), lessOrEqual(scanIdx, exprEnd), lessOrEqual(scanDepth, 64), decreases(exprEnd - scanIdx)>
    if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanIdx, 1, "(") then
      if scanDepth < 64 then scanDepth = scanDepth + 1 else ()
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanIdx, 1, ")") then
      if scanDepth > 0 then scanDepth = scanDepth - 1 else ()
    else if scanDepth == 0 then
      if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanIdx, 2, "||") then
        foundTopOr = true
      else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanIdx, 2, "if") then
        foundTopIf = true
      else
        ()
    else
      ()
    scanIdx = scanIdx + 1
  foundTopOr && !foundTopIf
