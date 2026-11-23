// prompt: check whether an identifier token begins an enum equality comparison and emit its opening wrapper.
def Stigi.Emit.tryStartEnumEqual {Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, cursor: U64, exprEnd: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, frameKinds: Array[U8](32), frameSubs: Array[U8](32), frameArgs: Array[U8](32), frameTargets: Array[U64](32), framePrev: Array[U64](32), topBox: Array[U64](1), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprEnd, tokCount), below(cursor, exprEnd), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val primEnd: U64 = Stigi.Emit.scanPrimaryEnd(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, exprEnd)
  val curTop: U64 = topBox(0)
  val alreadyInEqual: Bool = if curTop < 32 then frameKinds(curTop) == 22 && frameTargets(curTop) == primEnd else false
  if primEnd < exprEnd && !alreadyInEqual then
    val isEqualTok: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, primEnd, 2, "==")
    val isNeqTok: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, primEnd, 2, "!=")
    if isEqualTok || isNeqTok then
      val rhsStart: U64 = primEnd + 1
      val lhsEnum: U64 = Stigi.Emit.detectPrimaryEnumUnit(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, cursor, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount)
      val rhsEnum: U64 = if rhsStart < exprEnd then Stigi.Emit.detectPrimaryEnumUnit(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, rhsStart, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) else unitCount
      val matchEnum: U64 = if lhsEnum < unitCount then lhsEnum else rhsEnum
      if matchEnum < unitCount then
        val enumBeg: U64 = nameStarts(matchEnum)
        val enumFin: U64 = nameEnds(matchEnum)
        var pos: U64 = if isNeqTok then Stigi.Emit.writeLiteral(2, "(!", outCap, output, writeOffset) else Stigi.Emit.writeLiteral(1, "(", outCap, output, writeOffset)
        if enumBeg <= enumFin && enumFin <= indexCap then
          pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, enumBeg, enumFin, true, outCap, output, pos)
        else
          ()
        pos = Stigi.Emit.writeLiteral(5, "_eq((", outCap, output, pos)
        Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 22, 0, primEnd)
        pos
      else
        writeOffset
    else
      writeOffset
  else
    writeOffset
