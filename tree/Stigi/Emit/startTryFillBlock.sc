// prompt: lower an Array.tryFill match header into aligned allocation, fill loop, and tryFill block frame.
pub def Stigi.Emit.startTryFillBlock {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, lineStart: U64, lineEnd: U64, lineCol: U64, isTailFlag: U8, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), tempBox: Array[U64](1), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(lineStart, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val openParen: U64 = Stigi.Emit.stepOptionalBrace(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, lineStart + 1, lineEnd)
  val curLocs: U64 = localCountBox(0)
  val safeLocs: U64 = if curLocs <= localCap then curLocs else localCap
  val rawTemp: U64 = tempBox(0)
  val tempNum: U64 = if rawTemp < 65535 then rawTemp + 1 else 65535
  tempBox(0) := tempNum
  if openParen < lineEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, openParen, 1, "(") then
    val afterParen: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, openParen, 40, 41)
    val closeParen: U64 = if afterParen <= lineEnd && afterParen > openParen + 1 then afterParen - 1 else lineEnd
    val lenStart: U64 = openParen + 1
    val commaTok: U64 = if lenStart <= closeParen then Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, lenStart, closeParen, 1, ",") else closeParen
    val initStart: U64 = if commaTok < closeParen then commaTok + 1 else closeParen
    var elemBeg: U64 = 0
    var elemFin: U64 = 0
    if initStart < closeParen then
      val initTokBeg: U64 = tokStart(initStart)
      val initTokFin: U64 = tokEnd(initStart)
      if initTokBeg <= initTokFin && initTokFin <= srcCap then
        val initLoc: U64 = Stigi.Emit.findLocalIndex(srcCap, srcBytes, initTokBeg, initTokFin, localCap, localNameStart, localNameEnd, safeLocs)
        if initLoc < safeLocs then
          elemBeg = localTypeStart(initLoc)
          elemFin = localTypeEnd(initLoc)
        else
          ()
      else
        ()
    else
      ()
    var pos: U64 = Stigi.Emit.writeLine(1, "{", outCap, output, writeOffset)
    pos = Stigi.Emit.writeLiteral(14, "uint64_t _tlen", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(14, " = (uint64_t)(", outCap, output, pos)
    if lenStart < commaTok then
      pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, lenStart, commaTok, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(1, "0", outCap, output, pos)
    pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
    pos = Stigi.Emit.emitElemOrDefault(srcCap, srcBytes, elemBeg, elemFin, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(7, " _tinit", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
    if initStart < closeParen then
      pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, initStart, closeParen, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(1, "0", outCap, output, pos)
    pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
    pos = Stigi.Emit.emitElemOrDefault(srcCap, srcBytes, elemBeg, elemFin, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(7, " *_tptr", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
    pos = Stigi.Emit.emitElemOrDefault(srcCap, srcBytes, elemBeg, elemFin, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(27, " *)stigi_rt_try_alloc(_tlen", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(9, ", sizeof(", outCap, output, pos)
    pos = Stigi.Emit.emitElemOrDefault(srcCap, srcBytes, elemBeg, elemFin, outCap, output, pos)
    pos = Stigi.Emit.writeLine(3, "));", outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(9, "if (_tptr", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(18, " != NULL && _tinit", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(26, " != 0) { for (uint64_t _ti", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(9, " = 0; _ti", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(8, " < _tlen", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(5, "; _ti", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(9, "++) _tptr", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, "[_ti", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(10, "] = _tinit", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
    pos = Stigi.Emit.writeLine(3, "; }", outCap, output, pos)
    Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 5, lineCol, isTailFlag, safeLocs, tempNum, elemBeg, elemFin)
    pos
  else
    writeOffset
