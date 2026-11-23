// prompt: emit a C forward declaration or function signature header for an L0 def or Test unit.
pub def Stigi.Emit.emitFunctionHeader {Stigi.Mut} (indexCap: U64, indexBytes: Array[U8](indexCap), nameStart: U64, nameEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, paramListStart: U64, afterParams: U64, returnTok: U64, hasReturnTok: Bool, isForwardDecl: Bool, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(paramListStart, afterParams), lessOrEqual(afterParams, tokCount), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLiteral(7, "static ", outCap, output, writeOffset)
  if hasReturnTok && returnTok < tokCount then
    pos = Stigi.Emit.emitTypeName(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, returnTok, outCap, output, pos)
  else
    pos = Stigi.Emit.writeLiteral(4, "void", outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
  pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, nameStart, nameEnd, true, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(1, "(", outCap, output, pos)
  var emittedCount: U64 = 0
  if paramListStart < afterParams then
    var cursor: U64 = paramListStart + 1
    var stepBudget: U64 = 0
    while cursor < afterParams && stepBudget < tokCount do
      @<lessOrEqual(cursor, tokCount), lessOrEqual(stepBudget, tokCount), lessOrEqual(emittedCount, stepBudget), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(tokCount - stepBudget)>
      val nextStep: U64 = stepBudget + 1
      val colonTok: U64 = cursor + 1
      val typePos: U64 = cursor + 2
      if typePos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, colonTok, 1, ":") then
        if emittedCount > 0 then
          pos = Stigi.Emit.writeLiteral(2, ", ", outCap, output, pos)
        else
          ()
        pos = Stigi.Emit.emitTypeName(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, typePos, outCap, output, pos)
        pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
        val paramBeg: U64 = tokStart(cursor)
        val paramFin: U64 = tokEnd(cursor)
        if paramBeg <= paramFin && paramFin <= srcCap then
          pos = Stigi.Emit.writeSliceMangled(srcCap, srcBytes, paramBeg, paramFin, false, outCap, output, pos)
        else
          ()
        emittedCount = emittedCount + 1
        var nextPos: U64 = typePos + 1
        if nextPos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, "[") then
          nextPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextPos, 91, 93)
        else
          ()
        if nextPos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, "(") then
          nextPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextPos, 40, 41)
        else
          ()
        cursor = if nextPos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, ",") then nextPos + 1 else nextPos
      else
        cursor = cursor + 1
      stepBudget = nextStep
  else
    ()
  if emittedCount == 0 then
    pos = Stigi.Emit.writeLiteral(4, "void", outCap, output, pos)
  else
    ()
  if isForwardDecl then
    Stigi.Emit.writeLine(2, ");", outCap, output, pos)
  else
    Stigi.Emit.writeLine(3, ") {", outCap, output, pos)
