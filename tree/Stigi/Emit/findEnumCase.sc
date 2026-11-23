// prompt: resolve a short or qualified enum constructor slice against registered enum cases.
pub def Stigi.Emit.findEnumCase(unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), queryStart: U64, queryEnd: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(queryStart, queryEnd), lessOrEqual(queryEnd, srcCap), below(srcCap, 0x100000000), lessOrEqual(caseCount, caseCap), below(caseCap, 4096)>: Stigi.Emit.CaseMatch =
  var lastDot: U64 = queryEnd
  var scanPos: U64 = queryStart
  while scanPos < queryEnd do
    @<lessOrEqual(queryStart, scanPos), lessOrEqual(scanPos, queryEnd), lessOrEqual(queryStart, lastDot), lessOrEqual(lastDot, queryEnd), decreases(queryEnd - scanPos)>
    if srcBytes(scanPos) == 46 then
      lastDot = scanPos
    else
      ()
    scanPos = scanPos + 1
  val shortStart: U64 = if lastDot < queryEnd then lastDot + 1 else queryStart
  val callerSpaceLen: U64 = callerSpaceEnd - callerStart
  var bestCase: U64 = caseCount
  var bestEnum: U64 = unitCount
  var bestScore: U64 = 0
  var idx: U64 = 0
  while idx < caseCount do
    @<lessOrEqual(idx, caseCount), lessOrEqual(bestCase, caseCount), lessOrEqual(bestEnum, unitCount), decreases(caseCount - idx)>
    val nameBeg: U64 = caseNameStart(idx)
    val nameFin: U64 = caseNameEnd(idx)
    val ownerUnit: U64 = caseEnumUnit(idx)
    if nameBeg <= nameFin && nameFin <= srcCap && ownerUnit < unitCount then
      if Stigi.Emit.slicesEqual(srcCap, srcBytes, nameBeg, nameFin, srcCap, srcBytes, shortStart, queryEnd) then
        val enumBeg: U64 = nameStarts(ownerUnit)
        val enumFin: U64 = nameEnds(ownerUnit)
        if lastDot < queryEnd && enumBeg <= enumFin && enumFin <= indexCap then
          if Stigi.Emit.slicesEqual(indexCap, indexBytes, enumBeg, enumFin, srcCap, srcBytes, queryStart, lastDot) then
            bestCase = idx
            bestEnum = ownerUnit
            bestScore = 3
          else
            ()
        else if lastDot == queryEnd && bestScore < 2 then
          var sameSpace: Bool = false
          if callerSpaceLen > 0 && enumBeg <= enumFin && enumFin <= indexCap then
            val prefixEnd: U64 = enumBeg + callerSpaceLen
            if prefixEnd <= enumFin then
              sameSpace = Stigi.Emit.slicesEqual(indexCap, indexBytes, callerStart, callerSpaceEnd, indexCap, indexBytes, enumBeg, prefixEnd)
            else
              ()
          else
            ()
          val score: U64 = if sameSpace then 2 else 1
          if score > bestScore then
            bestCase = idx
            bestEnum = ownerUnit
            bestScore = score
          else
            ()
        else
          ()
      else
        ()
    else
      ()
    idx = idx + 1
  if bestCase < caseCount && bestEnum < unitCount then
    val localTag: U64 = Stigi.Emit.countEnumLocalTag(caseCap, caseEnumUnit, bestCase, bestEnum)
    CaseHit(true, bestCase, bestEnum, localTag)
  else
    CaseHit(false, 0, 0, 0)
