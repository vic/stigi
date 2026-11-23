// prompt: handle one case arm inside an enum match block binding payload fields and lowering inline arms.
def Stigi.Emit.handleEnumMatchCase {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, lineStart: U64, lineEnd: U64, lineCol: U64, retTypeTok: U64, hasRetTok: Bool, isVoidRet: Bool, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), targetBox: Array[U64](32), curTop: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(lineStart, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), below(curTop, 96), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val tempNum: U64 = blkTempNums(curTop)
  val isTailArm: Bool = blkFlags(curTop) == 1
  val savedLocs: U64 = localCountBox(0)
  val nameTok: U64 = lineStart + 1
  var pos: U64 = writeOffset
  if nameTok < lineEnd then
    val nameBeg: U64 = tokStart(nameTok)
    val nameFin: U64 = tokEnd(nameTok)
    if nameBeg <= nameFin && nameFin <= srcCap then
      var foundCaseIdx: U64 = caseCount
      var foundLocalTag: U64 = 0
      Stigi.Emit.findEnumCase(unitCount, indexCap, indexBytes, nameStarts, nameEnds, callerStart, callerSpaceEnd, srcCap, srcBytes, nameBeg, nameFin, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) match
        case CaseHit(found, hitIdx, _, hitTag) =>
          if found && hitIdx < caseCount then
            foundCaseIdx = hitIdx
            foundLocalTag = hitTag
          else
            ()
      pos = Stigi.Emit.writeLiteral(5, "case ", outCap, output, pos)
      pos = Stigi.Emit.writeUnsignedDecimal{region}(foundLocalTag, outCap, output, pos)
      pos = Stigi.Emit.writeLine(4, "U: {", outCap, output, pos)
      val openParen: U64 = nameTok + 1
      if foundCaseIdx < caseCount && openParen < lineEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, openParen, 1, "(") then
        val afterParen: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, openParen, 40, 41)
        val closeParen: U64 = if afterParen <= lineEnd then afterParen else lineEnd
        pos = Stigi.Emit.emitPatternBindings{region}(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, openParen, closeParen, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, caseFieldBase(foundCaseIdx), caseFieldCount(foundCaseIdx), foundLocalTag, true, tempNum, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, outCap, output, pos)
      else
        ()
      val arrowTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nameTok, lineEnd, 2, "=>")
      val bodyStart: U64 = arrowTok + 1
      if bodyStart < lineEnd then
        val liveLocs: U64 = localCountBox(0)
        val safeLocs: U64 = if liveLocs <= localCap then liveLocs else localCap
        if isTailArm && !isVoidRet then
          pos = Stigi.Emit.emitTailWithTarget{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, bodyStart, lineEnd, retTypeTok, hasRetTok, isVoidRet, targetBox, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkTempNums, blkPrev, blkTopBox(0), outCap, output, pos)
        else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, bodyStart, 2, "if") then
          pos = Stigi.Emit.emitSingleLineIfStmt{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, bodyStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
        else
          pos = Stigi.Emit.emitSimpleStmt{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, bodyStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
        localCountBox(0) := savedLocs
        Stigi.Emit.writeLine(8, "break; }", outCap, output, pos)
      else
        Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 9, lineCol, if isTailArm then 1 else 0, savedLocs, tempNum, 0, 0)
        pos
    else
      pos
  else
    pos
