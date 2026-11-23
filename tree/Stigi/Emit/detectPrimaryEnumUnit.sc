// prompt: detect whether the primary expression starting at a token has a registered enum type.
pub def Stigi.Emit.detectPrimaryEnumUnit(unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokIdx: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), below(tokIdx, tokCap), below(tokCap, 65536), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096)>: U64 =
  val kindByte: U8 = tokKind(tokIdx)
  if kindByte != 1 && kindByte != 2 then
    unitCount
  else
    val tokBeg: U64 = tokStart(tokIdx)
    val tokFin: U64 = tokEnd(tokIdx)
    if tokBeg <= tokFin && tokFin <= srcCap then
      val locIdx: U64 = Stigi.Emit.findLocalIndex(srcCap, srcBytes, tokBeg, tokFin, localCap, localNameStart, localNameEnd, localCount)
      if locIdx < localCount then
        if localIsArray(locIdx) == 0 then
          val locTypeBeg: U64 = localTypeStart(locIdx)
          val locTypeFin: U64 = localTypeEnd(locIdx)
          if locTypeBeg <= locTypeFin && locTypeFin <= srcCap && !Stigi.Emit.isPrimitiveTypeSlice(srcCap, srcBytes, locTypeBeg, locTypeFin) then
            val foundEnum: U64 = Stigi.Emit.findUnitByName(unitCount, indexCap, indexBytes, nameStarts, nameEnds, srcCap, srcBytes, locTypeBeg, locTypeFin)
            if foundEnum < unitCount && kindCodes(foundEnum) == 1 then foundEnum else unitCount
          else
            unitCount
        else
          unitCount
      else
        val unitHit: U64 = Stigi.Emit.findUnitByName(unitCount, indexCap, indexBytes, nameStarts, nameEnds, srcCap, srcBytes, tokBeg, tokFin)
        if unitHit < unitCount && kindCodes(unitHit) == 3 then
          val retEnum: U64 = unitRetEnum(unitHit)
          if retEnum < unitCount then retEnum else unitCount
        else
          Stigi.Emit.findEnumCase(unitCount, indexCap, indexBytes, nameStarts, nameEnds, callerStart, callerSpaceEnd, srcCap, srcBytes, tokBeg, tokFin, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) match
            case CaseHit(caseFound, _, enumHit, _) => if caseFound && enumHit < unitCount then enumHit else unitCount
    else
      unitCount
