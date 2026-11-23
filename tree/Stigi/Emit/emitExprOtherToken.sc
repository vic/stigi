// prompt: lower one non-identifier token inside an L0 expression using the expression frame stack.
def Stigi.Emit.emitExprOtherToken {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, cursor: U64, exprEnd: U64, frameKinds: Array[U8](32), frameSubs: Array[U8](32), frameArgs: Array[U8](32), frameTargets: Array[U64](32), framePrev: Array[U64](32), topBox: Array[U64](1), cursorBox: Array[U64](1), wrapTopOr: Bool, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprEnd, tokCount), below(cursor, exprEnd), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val kindByte: U8 = tokKind(cursor)
  val tokBeg: U64 = tokStart(cursor)
  val tokFin: U64 = tokEnd(cursor)
  val nextTok: U64 = cursor + 1
  cursorBox(0) := nextTok
  if kindByte == 7 then
    writeOffset
  else if kindByte == 3 then
    if tokBeg <= tokFin && tokFin <= srcCap then Stigi.Emit.emitNumberLiteral(srcCap, srcBytes, tokBeg, tokFin, outCap, output, writeOffset) else writeOffset
  else if kindByte == 4 || kindByte == 5 then
    var pos: U64 = writeOffset
    if tokBeg <= tokFin && tokFin <= srcCap then
      pos = if kindByte == 5 then Stigi.Emit.writeLiteral(12, "((uint8_t *)", outCap, output, pos) else Stigi.Emit.writeLiteral(10, "((uint8_t)", outCap, output, pos)
      pos = Stigi.Print.writeSlice(srcCap, srcBytes, tokBeg, tokFin, outCap, output, pos)
      pos = Stigi.Emit.writeLiteral(1, ")", outCap, output, pos)
    else
      ()
    pos
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "(") then
    if nextTok < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextTok, 1, ")") then
      cursorBox(0) := nextTok + 1
      Stigi.Emit.writeLiteral(7, "(void)0", outCap, output, writeOffset)
    else
      Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 1, 0, 0)
      Stigi.Emit.writeLiteral(1, "(", outCap, output, writeOffset)
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, ",") then
    var pos: U64 = Stigi.Emit.popInlineElseFrames(frameKinds, framePrev, topBox, outCap, output, writeOffset)
    val curTop: U64 = topBox(0)
    if curTop < 32 then
      val curArg: U8 = frameArgs(curTop)
      pos = Stigi.Emit.emitBuiltinComma(frameKinds(curTop), frameSubs(curTop), curArg, outCap, output, pos)
      if curArg < 16 then frameArgs(curTop) := U8.ofU64(U64.ofU8(curArg) + 1) else ()
    else
      pos = Stigi.Emit.writeLiteral(2, ", ", outCap, output, pos)
    pos
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, ")") then
    var pos: U64 = Stigi.Emit.popInlineElseFrames(frameKinds, framePrev, topBox, outCap, output, writeOffset)
    val curTop: U64 = topBox(0)
    if curTop < 32 then
      pos = Stigi.Emit.emitBuiltinClose(frameKinds(curTop), outCap, output, pos)
      topBox(0) := framePrev(curTop)
    else
      pos = Stigi.Emit.writeLiteral(1, ")", outCap, output, pos)
    pos
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 2, "||") then
    if wrapTopOr && topBox(0) == 32 then
      Stigi.Emit.writeLiteral(6, ") || (", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(4, " || ", outCap, output, writeOffset)
  else
    var pos: U64 = writeOffset
    if tokBeg <= tokFin && tokFin <= srcCap then
      if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "!") then
        pos = Stigi.Emit.writeLiteral(1, "!", outCap, output, pos)
      else
        pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
        pos = Stigi.Print.writeSlice(srcCap, srcBytes, tokBeg, tokFin, outCap, output, pos)
        pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
    else
      ()
    pos
