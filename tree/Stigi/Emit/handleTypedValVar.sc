// prompt: lower a typed val or var declaration with match, Array.tryFill, multi-line if, or scalar RHS.
def Stigi.Emit.handleTypedValVar {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, lineStart: U64, lineEnd: U64, lineCol: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCountBox: Array[U64](1), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, blkKinds: Array[U8](96), blkCols: Array[U64](96), blkFlags: Array[U8](96), blkSavedLocals: Array[U64](96), blkTempNums: Array[U64](96), blkAuxStart: Array[U64](96), blkAuxEnd: Array[U64](96), blkPrev: Array[U64](96), blkTopBox: Array[U64](1), tempBox: Array[U64](1), targetBox: Array[U64](32), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(lineStart, lineEnd), lessOrEqual(lineEnd, tokCount), below(srcCap, 0x100000000), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val nameTok: U64 = lineStart + 1
  val typeTok: U64 = lineStart + 3
  val curLocs: U64 = localCountBox(0)
  val safeLocs: U64 = if curLocs <= localCap then curLocs else localCap
  if typeTok < lineEnd then
    val equalTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, typeTok, lineEnd, 1, "=")
    val nameBeg: U64 = tokStart(nameTok)
    val nameFin: U64 = tokEnd(nameTok)
    val isArrayType: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, typeTok, 5, "Array")
    val elemTok: U64 = typeTok + 2
    var pos: U64 = Stigi.Emit.emitTypeName(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, typeTok, outCap, output, writeOffset)
    pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
    if nameBeg <= nameFin && nameFin <= srcCap then
      pos = Stigi.Print.writeSlice(srcCap, srcBytes, nameBeg, nameFin, outCap, output, pos)
    else
      ()
    val rhsStart: U64 = equalTok + 1
    var isBlockRhs: Bool = false
    if rhsStart < lineEnd then
      val matchTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, rhsStart, lineEnd, 5, "match")
      if matchTok + 1 == lineEnd then
        isBlockRhs = true
      else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, rhsStart, 2, "if") && rhsStart + 1 < lineEnd then
        val thenTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, rhsStart + 1, lineEnd, 4, "then")
        if thenTok < lineEnd && thenTok + 1 == lineEnd then isBlockRhs = true else ()
      else
        ()
    else
      ()
    if isBlockRhs && rhsStart < lineEnd then
      pos = Stigi.Emit.writeLiteral(14, " = {0}; (void)", outCap, output, pos)
      if nameBeg <= nameFin && nameFin <= srcCap then
        pos = Stigi.Print.writeSlice(srcCap, srcBytes, nameBeg, nameFin, outCap, output, pos)
      else
        ()
      pos = Stigi.Emit.writeLine(1, ";", outCap, output, pos)
      if safeLocs < localCap then
        localNameStart(safeLocs) := nameBeg
        localNameEnd(safeLocs) := nameFin
        localIsArray(safeLocs) := if isArrayType then 1 else 0
        if isArrayType && elemTok < lineEnd then
          localTypeStart(safeLocs) := tokStart(elemTok)
          localTypeEnd(safeLocs) := tokEnd(elemTok)
        else
          localTypeStart(safeLocs) := tokStart(typeTok)
          localTypeEnd(safeLocs) := tokEnd(typeTok)
        localCountBox(0) := safeLocs + 1
      else
        ()
      if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, rhsStart, 13, "Array.tryFill") then
        pos = Stigi.Emit.startTryFillBlock{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, rhsStart, lineEnd, lineCol, 1, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, outCap, output, pos)
      else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, rhsStart, 2, "if") && rhsStart + 1 < lineEnd then
        val condStart: U64 = rhsStart + 1
        val thenTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, condStart, lineEnd, 4, "then")
        val afterLocs: U64 = localCountBox(0)
        val safeAfter: U64 = if afterLocs <= localCap then afterLocs else localCap
        pos = Stigi.Emit.writeLiteral(4, "if (", outCap, output, pos)
        pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, condStart, thenTok, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeAfter, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
        pos = Stigi.Emit.writeLine(3, ") {", outCap, output, pos)
        Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 3, lineCol, 1, safeAfter, 0, 0, 0)
      else
        val matchTok: U64 = Stigi.Emit.findTokenInLine(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, rhsStart, lineEnd, 5, "match")
        pos = Stigi.Emit.startMatchBlock{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, rhsStart, matchTok, lineEnd, lineCol, 1, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, outCap, output, pos)
      Stigi.Emit.pushTargetEntry(targetBox, nameBeg, nameFin, blkTopBox(0))
      pos
    else
      pos = Stigi.Emit.writeLiteral(4, " = (", outCap, output, pos)
      if rhsStart < lineEnd then
        pos = Stigi.Emit.emitExprRange{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, rhsStart, lineEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, safeLocs, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, outCap, output, pos)
      else
        pos = Stigi.Emit.writeLiteral(1, "0", outCap, output, pos)
      pos = Stigi.Emit.writeLiteral(9, "); (void)", outCap, output, pos)
      if nameBeg <= nameFin && nameFin <= srcCap then
        pos = Stigi.Print.writeSlice(srcCap, srcBytes, nameBeg, nameFin, outCap, output, pos)
      else
        ()
      pos = Stigi.Emit.writeLine(1, ";", outCap, output, pos)
      if safeLocs < localCap then
        localNameStart(safeLocs) := nameBeg
        localNameEnd(safeLocs) := nameFin
        localIsArray(safeLocs) := if isArrayType then 1 else 0
        if isArrayType && elemTok < lineEnd then
          localTypeStart(safeLocs) := tokStart(elemTok)
          localTypeEnd(safeLocs) := tokEnd(elemTok)
        else
          localTypeStart(safeLocs) := tokStart(typeTok)
          localTypeEnd(safeLocs) := tokEnd(typeTok)
        localCountBox(0) := safeLocs + 1
      else
        ()
      pos
  else
    writeOffset
