// prompt: check whether every payload enum type of an enum unit is already emitted.
def Stigi.Emit.enumDepsReady(unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), enumEmitted: Array[U8](unitCount), enumUnit: U64, srcCap: U64, srcBytes: Array[U8](srcCap), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64) <below(enumUnit, unitCount), below(unitCount, 4096), below(indexCap, 0x100000000), below(srcCap, 0x100000000), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096)>: Bool =
  var ready: Bool = true
  var caseIdx: U64 = 0
  while caseIdx < caseCount && ready do
    @<lessOrEqual(caseIdx, caseCount), decreases(caseCount - caseIdx)>
    if caseEnumUnit(caseIdx) == enumUnit then
      val fieldBase: U64 = caseFieldBase(caseIdx)
      val fieldLen: U64 = caseFieldCount(caseIdx)
      if fieldLen <= fieldCount && fieldBase < fieldCount then
        var fieldPos: U64 = fieldBase
        var fieldStep: U64 = 0
        while fieldStep < fieldLen && fieldPos < fieldCount && ready do
          @<lessOrEqual(fieldStep, fieldLen), lessOrEqual(fieldStep, fieldCount), lessOrEqual(fieldPos, fieldCount), decreases(fieldCount - fieldStep)>
          val typeStart: U64 = fieldTypeStart(fieldPos)
          val typeEnd: U64 = fieldTypeEnd(fieldPos)
          if typeStart <= typeEnd && typeEnd <= srcCap then
            if !Stigi.Emit.isTypeSliceEmitted(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, enumEmitted, srcCap, srcBytes, typeStart, typeEnd) then
              ready = false
            else
              ()
          else
            ()
          fieldPos = fieldPos + 1
          fieldStep = fieldStep + 1
      else
        ()
    else
      ()
    caseIdx = caseIdx + 1
  ready
