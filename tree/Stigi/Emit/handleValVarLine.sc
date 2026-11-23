// prompt: dispatch single-case enum destructuring val lines or typed val and var declarations.
def Stigi.Emit.handleValVarLine {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, lineStart: U64, lineEnd: U64, lineCol: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), tempBox: Array[U64](1), targetBox: Array[U64](32), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(lineStart, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val nameTok: U64 = lineStart + 1
  val secondTok: U64 = lineStart + 2
  val curLocs: U64 = localCountBox(0)
  val safeLocs: U64 = if curLocs <= localCap then curLocs else localCap
  if secondTok < lineEnd && tokKind(nameTok) == 2 && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, secondTok, 1, "(") then
    val afterParen: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, secondTok, 40, 41)
    val closeParen: U64 = if afterParen <= lineEnd then afterParen else lineEnd
    val equalTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, closeParen, lineEnd, 1, "=")
    val ctorBeg: U64 = tokStart(nameTok)
    val ctorFin: U64 = tokEnd(nameTok)
    var foundCaseIdx: U64 = caseCount
    var foundEnumIdx: U64 = unitCount
    var foundLocalTag: U64 = 0
    if ctorBeg <= ctorFin && ctorFin <= srcCap then
      Stigi.Emit.findEnumCase(unitCount, indexCap, indexBytes, nameStarts, nameEnds, callerStart, callerSpaceEnd, srcCap, srcBytes, ctorBeg, ctorFin, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) match
        case CaseHit(found, hitIdx, hitEnum, hitTag) =>
          if found && hitIdx < caseCount && hitEnum < unitCount then
            foundCaseIdx = hitIdx
            foundEnumIdx = hitEnum
            foundLocalTag = hitTag
          else
            ()
    else
      ()
    val rawTemp: U64 = tempBox(0)
    val tempNum: U64 = if rawTemp < 65535 then rawTemp + 1 else 65535
    tempBox(0) := tempNum
    var pos: U64 = writeOffset
    if foundEnumIdx < unitCount then
      val enumBeg: U64 = nameStarts(foundEnumIdx)
      val enumFin: U64 = nameEnds(foundEnumIdx)
      if enumBeg <= enumFin && enumFin <= indexCap then
        pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, enumBeg, enumFin, true, outCap, output, pos)
      else
        pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, " _ds", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
    if equalTok + 1 < lineEnd then
      pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, equalTok + 1, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(1, "0", outCap, output, pos)
    pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
    if foundCaseIdx < caseCount then
      Stigi.Emit.emitPatternBindings{region}(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, secondTok, closeParen, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, caseFieldBase(foundCaseIdx), caseFieldCount(foundCaseIdx), foundLocalTag, false, tempNum, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, outCap, output, pos)
    else
      pos
  else
    Stigi.Emit.handleTypedValVar{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lineStart, lineEnd, lineCol, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, targetBox, outCap, output, writeOffset)
