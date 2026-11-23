// prompt: scan the token stream of a def or val unit to locate parameters, return type, and body start.
pub def Stigi.Emit.scanDefSignature(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(srcCap, 0x100000000)>: Stigi.Emit.SigOffsets =
  var cursor: U64 = 0
  var headTok: U64 = tokCount
  var foundHead: Bool = false
  while cursor < tokCount && !foundHead do
    @<lessOrEqual(cursor, tokCount), lessOrEqual(headTok, tokCount), decreases(tokCount - cursor)>
    val isDef: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 3, "def")
    val isVal: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 3, "val")
    if isDef || isVal then
      headTok = cursor
      foundHead = true
    else
      ()
    cursor = cursor + 1
  var scanPos: U64 = if headTok + 2 <= tokCount then headTok + 2 else tokCount
  if scanPos < tokCount && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "{") then
    scanPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, scanPos, 123, 125)
  else
    ()
  val paramListStart: U64 = scanPos
  val afterParams: U64 = if scanPos < tokCount && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "(") then Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, scanPos, 40, 41) else scanPos
  scanPos = afterParams
  if scanPos < tokCount && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "<") then
    scanPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, scanPos, 60, 62)
  else
    ()
  var hasReturnTok: Bool = false
  var returnTok: U64 = 0
  if scanPos < tokCount && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, ":") then
    val nextOne: U64 = scanPos + 1
    if nextOne < tokCount then
      hasReturnTok = true
      returnTok = nextOne
      scanPos = nextOne + 1
    else
      ()
  else
    ()
  var bodyTok: U64 = tokCount
  var foundAssign: Bool = false
  var assignStep: U64 = 0
  while scanPos < tokCount && !foundAssign && assignStep < tokCount do
    @<lessOrEqual(scanPos, tokCount), lessOrEqual(bodyTok, tokCount), lessOrEqual(assignStep, tokCount), decreases(tokCount - assignStep)>
    if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "=") then
      foundAssign = true
      bodyTok = scanPos + 1
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "<") then
      scanPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, scanPos, 60, 62)
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "(") then
      scanPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, scanPos, 40, 41)
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, scanPos, 1, "[") then
      scanPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, scanPos, 91, 93)
    else
      scanPos = scanPos + 1
    assignStep = assignStep + 1
  DefSignature(paramListStart, afterParams, returnTok, hasReturnTok, bodyTok)
