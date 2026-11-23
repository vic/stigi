// prompt: dispatch one tokenized body line inside a def or Test function body.
def Stigi.Emit.emitBodyLine {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCol: Array[U64](tokCap), tokCount: U64, lineStart: U64, lineEnd: U64, lineCol: U64, retTypeTok: U64, hasRetTok: Bool, isVoidRet: Bool, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), tempBox: Array[U64](1), targetBox: Array[U64](32), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(lineStart, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val curTop: U64 = blkTopBox(0)
  val curLocs: U64 = localCountBox(0)
  val safeLocs: U64 = if curLocs <= localCap then curLocs else localCap
  val parentFlags: U8 = if curTop < 96 then blkFlags(curTop) else 0
  val parentTail: Bool = parentFlags == 1 || parentFlags == 5
  if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 1, "@") then
    writeOffset
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 4, "case") then
    if curTop < 96 then
      if blkKinds(curTop) == 5 then
        Stigi.Emit.handleTryFillCase{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, lineCol, retTypeTok, hasRetTok, isVoidRet, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, targetBox, curTop, outCap, output, writeOffset)
      else if blkKinds(curTop) == 8 then
        Stigi.Emit.handleEnumMatchCase{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, lineCol, retTypeTok, hasRetTok, isVoidRet, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseFieldBase, caseFieldCount, caseCount, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, targetBox, curTop, outCap, output, writeOffset)
      else
        writeOffset
    else
      writeOffset
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 4, "else") then
    if curTop < 96 then localCountBox(0) := blkSavedLocals(curTop) else ()
    val afterElse: U64 = lineStart + 1
    if afterElse < lineEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, afterElse, 2, "if") then
      val condStart: U64 = afterElse + 1
      val thenTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, condStart, lineEnd, 4, "then")
      var pos: U64 = Stigi.Emit.writeLiteral(11, "} else if (", outCap, output, writeOffset)
      pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, condStart, thenTok, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
      Stigi.Emit.writeLine(3, ") {", outCap, output, pos)
    else
      if curTop < 96 then blkKinds(curTop) := 4 else ()
      Stigi.Emit.writeLine(8, "} else {", outCap, output, writeOffset)
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 5, "while") then
    val condStart: U64 = lineStart + 1
    val whileEndTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, condStart, lineEnd, 2, "do")
    var pos: U64 = Stigi.Emit.writeLiteral(7, "while (", outCap, output, writeOffset)
    pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, condStart, whileEndTok, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    pos = Stigi.Emit.writeLine(3, ") {", outCap, output, pos)
    Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 2, lineCol, 0, safeLocs, 0, 0, 0)
    pos
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 2, "if") then
    val condStart: U64 = lineStart + 1
    val thenTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, condStart, lineEnd, 4, "then")
    val hasSib: Bool = Stigi.Emit.hasSiblingAfterBlock(srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCol, tokCount, lineEnd, lineCol)
    if thenTok < lineEnd && thenTok + 1 == lineEnd then
      var pos: U64 = Stigi.Emit.writeLiteral(4, "if (", outCap, output, writeOffset)
      pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, condStart, thenTok, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
      pos = Stigi.Emit.writeLine(3, ") {", outCap, output, pos)
      Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 3, lineCol, if parentTail && !hasSib then 1 else 0, safeLocs, 0, 0, 0)
      pos
    else if parentTail && !hasSib && !isVoidRet then
      Stigi.Emit.emitTailWithTarget{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, retTypeTok, hasRetTok, isVoidRet, targetBox, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkTempNums, blkPrev, curTop, outCap, output, writeOffset)
    else
      Stigi.Emit.emitSingleLineIfStmt{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, writeOffset)
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 3, "val") || Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 3, "var") then
    Stigi.Emit.handleValVarLine{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, lineCol, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseFieldBase, caseFieldCount, caseCount, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, targetBox, outCap, output, writeOffset)
  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lineStart, 13, "Array.tryFill") then
    val hasSib: Bool = Stigi.Emit.hasSiblingAfterBlock(srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCol, tokCount, lineEnd, lineCol)
    Stigi.Emit.startTryFillBlock{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, lineCol, if parentTail && !hasSib then 1 else 0, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, outCap, output, writeOffset)
  else
    val matchTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, lineStart, lineEnd, 5, "match")
    val hasSib: Bool = Stigi.Emit.hasSiblingAfterBlock(srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCol, tokCount, lineEnd, lineCol)
    if matchTok < lineEnd && matchTok + 1 == lineEnd then
      Stigi.Emit.startMatchBlock{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, matchTok, lineEnd, lineCol, if parentTail && !hasSib then 1 else 0, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, outCap, output, writeOffset)
    else if parentTail && !hasSib && !isVoidRet then
      Stigi.Emit.emitTailWithTarget{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, retTypeTok, hasRetTok, isVoidRet, targetBox, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkTempNums, blkPrev, curTop, outCap, output, writeOffset)
    else
      Stigi.Emit.emitSimpleStmt{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, writeOffset)
