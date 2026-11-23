// prompt: lower an enum match header by inferring the scrutinee enum type from the first case arm.
pub def Stigi.Emit.startMatchBlock {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, scrutStart: U64, matchTok: U64, lineEnd: U64, lineCol: U64, isTailFlag: U8, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), tempBox: Array[U64](1), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(scrutStart, matchTok), lessOrEqual(matchTok, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var scanCase: U64 = lineEnd
  while scanCase < tokCount && tokKind(scanCase) == 7 do
    @<lessOrEqual(lineEnd, scanCase), lessOrEqual(scanCase, tokCount), decreases(tokCount - scanCase)>
    scanCase = scanCase + 1
  var enumUnit: U64 = unitCount
  val caseNameTok: U64 = scanCase + 1
  if caseNameTok < tokCount then
    val nameBeg: U64 = tokStart(caseNameTok)
    val nameFin: U64 = tokEnd(caseNameTok)
    if nameBeg <= nameFin && nameFin <= srcCap then
      Stigi.Emit.findEnumCase(unitCount, indexCap, indexBytes, nameStarts, nameEnds, callerStart, callerSpaceEnd, srcCap, srcBytes, nameBeg, nameFin, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) match
        case CaseHit(found, _, foundEnum, _) =>
          if found && foundEnum < unitCount then enumUnit = foundEnum else ()
    else
      ()
  else
    ()
  val curLocs: U64 = localCountBox(0)
  val safeLocs: U64 = if curLocs <= localCap then curLocs else localCap
  val rawTemp: U64 = tempBox(0)
  val tempNum: U64 = if rawTemp < 65535 then rawTemp + 1 else 65535
  tempBox(0) := tempNum
  var pos: U64 = Stigi.Emit.writeLine(1, "{", outCap, output, writeOffset)
  if enumUnit < unitCount then
    val enumBeg: U64 = nameStarts(enumUnit)
    val enumFin: U64 = nameEnds(enumUnit)
    if enumBeg <= enumFin && enumFin <= indexCap then
      pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, enumBeg, enumFin, true, outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
  else
    pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(4, " _sc", outCap, output, pos)
  pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
  pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, scrutStart, matchTok, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
  pos = Stigi.Emit.writeLine(2, ");", outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(11, "switch (_sc", outCap, output, pos)
  pos = Stigi.Emit.writeUnsignedDecimal{region}(tempNum, outCap, output, pos)
  pos = Stigi.Emit.writeLine(7, ".tag) {", outCap, output, pos)
  Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 8, lineCol, isTailFlag, safeLocs, tempNum, enumUnit, 0)
  pos
