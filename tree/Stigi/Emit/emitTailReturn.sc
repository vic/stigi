// prompt: emit a tail return or target assignment expression freeing any active tryFill allocations.
pub def Stigi.Emit.emitTailReturn {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, exprStart: U64, exprEnd: U64, retTypeTok: U64, hasRetTok: Bool, isVoidRet: Bool, hasTargetVar: Bool, targetBeg: U64, targetFin: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, blkKinds: Array[U8](96), blkTempNums: Array[U64](96), blkPrev: Array[U64](96), blkTop: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprStart, exprEnd), lessOrEqual(exprEnd, tokCount), lessOrEqual(targetBeg, targetFin), lessOrEqual(targetFin, srcCap), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if hasTargetVar then
    var pos: U64 = Stigi.Print.writeSlice(srcCap, srcBytes, targetBeg, targetFin, outCap, output, writeOffset)
    pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
    pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, exprStart, exprEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    Stigi.Emit.writeLine(2, ");", outCap, output, pos)
  else if isVoidRet then
    var pos: U64 = Stigi.Emit.writeLiteral(7, "(void)(", outCap, output, writeOffset)
    pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, exprStart, exprEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    Stigi.Emit.writeLine(2, ");", outCap, output, pos)
  else if Stigi.Emit.hasActiveAlloc(blkKinds, blkPrev, blkTop) then
    var pos: U64 = Stigi.Emit.writeLiteral(2, "{ ", outCap, output, writeOffset)
    if hasRetTok && retTypeTok < tokCount then
      pos = Stigi.Emit.emitTypeName(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, retTypeTok, outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(8, " _rv = (", outCap, output, pos)
    pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, exprStart, exprEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
    pos = Stigi.Emit.emitActiveFrees{region}(blkKinds, blkTempNums, blkPrev, blkTop, outCap, output, pos)
    Stigi.Emit.writeLine(13, "return _rv; }", outCap, output, pos)
  else
    var pos: U64 = Stigi.Emit.writeLiteral(8, "return (", outCap, output, writeOffset)
    pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, exprStart, exprEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    Stigi.Emit.writeLine(2, ");", outCap, output, pos)
