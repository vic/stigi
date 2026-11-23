// prompt: handle one case arm inside an Array.tryFill match block.
def Stigi.Emit.handleTryFillCase {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, lineStart: U64, lineEnd: U64, lineCol: U64, retTypeTok: U64, hasRetTok: Bool, isVoidRet: Bool, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), targetBox: Array[U64](32), curTop: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(lineStart, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), below(curTop, 96), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val tempNum: U64 = blkTempNums(curTop)
  val elemBeg: U64 = blkAuxStart(curTop)
  val elemFin: U64 = blkAuxEnd(curTop)
  val curFlags: U8 = blkFlags(curTop)
  val isTailArm: Bool = curFlags == 1 || curFlags == 5
  val isFirstArm: Bool = curFlags < 4
  val armNameTok: U64 = lineStart + 1
  val isFilled: Bool = if armNameTok < lineEnd then Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, armNameTok, 6, "Filled") else false
  var pos: U64 = writeOffset
  if isFirstArm then
    blkFlags(curTop) := if isTailArm then 5 else 4
    pos = Stigi.Emit.writeLiteral(9, "if (_tptr", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = if isFilled then Stigi.Emit.writeLine(11, " != NULL) {", outCap, output, pos) else Stigi.Emit.writeLine(11, " == NULL) {", outCap, output, pos)
  else
    pos = Stigi.Emit.writeLine(6, "else {", outCap, output, pos)
  val savedLocs: U64 = localCountBox(0)
  val arrTok: U64 = lineStart + 3
  if isFilled && arrTok < lineEnd then
    val arrBeg: U64 = tokStart(arrTok)
    val arrFin: U64 = tokEnd(arrTok)
    if arrBeg <= arrFin && arrFin <= srcCap then
      pos = Stigi.Emit.emitElemOrDefault(srcCap, srcBytes, elemBeg, elemFin, outCap, output, pos)
      pos = Stigi.Emit.writeLiteral(2, " *", outCap, output, pos)
      pos = Stigi.Print.writeSlice(srcCap, srcBytes, arrBeg, arrFin, outCap, output, pos)
      pos = Stigi.Emit.writeLiteral(8, " = _tptr", outCap, output, pos)
      pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
      pos = Stigi.Emit.writeLiteral(8, "; (void)", outCap, output, pos)
      pos = Stigi.Print.writeSlice(srcCap, srcBytes, arrBeg, arrFin, outCap, output, pos)
      pos = Stigi.Emit.writeLine(1, ";", outCap, output, pos)
      val curLoc: U64 = localCountBox(0)
      if curLoc < localCap then
        localNameStart(curLoc) := arrBeg
        localNameEnd(curLoc) := arrFin
        localTypeStart(curLoc) := elemBeg
        localTypeEnd(curLoc) := elemFin
        localIsArray(curLoc) := 1
        localCountBox(0) := curLoc + 1
      else
        ()
    else
      ()
  else
    ()
  val arrowTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, armNameTok, lineEnd, 2, "=>")
  val bodyStart: U64 = arrowTok + 1
  if bodyStart < lineEnd then
    val liveLocs: U64 = localCountBox(0)
    val safeLocs: U64 = if liveLocs <= localCap then liveLocs else localCap
    if isFilled then
      Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 7, lineCol, if isTailArm then 1 else 0, savedLocs, tempNum, 0, 0)
    else
      ()
    if isTailArm && !isVoidRet then
      pos = Stigi.Emit.emitTailWithTarget{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, bodyStart, lineEnd, retTypeTok, hasRetTok, isVoidRet, targetBox, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkTempNums, blkPrev, blkTopBox(0), outCap, output, pos)
    else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, bodyStart, 2, "if") then
      pos = Stigi.Emit.emitSingleLineIfStmt{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, bodyStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    else
      pos = Stigi.Emit.emitSimpleStmt{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, bodyStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    if isFilled then
      val tmpTop: U64 = blkTopBox(0)
      if tmpTop < 96 then blkTopBox(0) := blkPrev(tmpTop) else ()
      if !isTailArm || isVoidRet || targetBox(0) > 0 then
        pos = Stigi.Emit.writeLiteral(10, "free(_tptr", outCap, output, pos)
        pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
        pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
      else
        ()
    else
      ()
    localCountBox(0) := savedLocs
    Stigi.Emit.writeLine(1, "}", outCap, output, pos)
  else
    Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, if isFilled then 7 else 6, lineCol, if isTailArm then 1 else 0, savedLocs, tempNum, 0, 0)
    pos
