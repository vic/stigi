// prompt: emit a single-line statement handling unit literals, array writes, reassignments, and calls.
def Stigi.Emit.emitSimpleStmt {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, stmtStart: U64, stmtEnd: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(stmtStart, stmtEnd), lessOrEqual(stmtEnd, tokCount), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if stmtStart >= stmtEnd then
    writeOffset
  else
    val nextTok: U64 = stmtStart + 1
    if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, stmtStart, 1, "(") && nextTok < stmtEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextTok, 1, ")") then
      Stigi.Emit.writeLine(8, "(void)0;", outCap, output, writeOffset)
    else
      val lhsEnd: U64 = Stigi.Emit.scanPrimaryEnd(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, stmtStart, stmtEnd)
      if lhsEnd < stmtEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, lhsEnd, 2, ":=") then
        var pos: U64 = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, stmtStart, lhsEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, writeOffset)
        pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
        pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lhsEnd + 1, stmtEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
        Stigi.Emit.writeLine(2, ");", outCap, output, pos)
      else if nextTok < stmtEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextTok, 1, "=") then
        val nameBeg: U64 = tokStart(stmtStart)
        val nameFin: U64 = tokEnd(stmtStart)
        var pos: U64 = if nameBeg <= nameFin && nameFin <= srcCap then Stigi.Print.writeSlice(srcCap, srcBytes, nameBeg, nameFin, outCap, output, writeOffset) else writeOffset
        pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
        pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, nextTok + 1, stmtEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
        Stigi.Emit.writeLine(2, ");", outCap, output, pos)
      else
        val pos: U64 = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, stmtStart, stmtEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, writeOffset)
        Stigi.Emit.writeLine(1, ";", outCap, output, pos)
