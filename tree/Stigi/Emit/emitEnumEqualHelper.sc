// prompt: emit the structural equality helper for one registered L0 enum unit.
def Stigi.Emit.emitEnumEqualHelper {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), enumUnit: U64, srcCap: U64, srcBytes: Array[U8](srcCap), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(enumUnit, unitCount), below(unitCount, 4096), below(indexCap, 0x100000000), below(srcCap, 0x100000000), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val nameStart: U64 = nameStarts(enumUnit)
  val nameEnd: U64 = nameEnds(enumUnit)
  if nameStart <= nameEnd && nameEnd <= indexCap then
    var pos: U64 = Stigi.Emit.writeLiteral(19, "static inline bool ", outCap, output, writeOffset)
    pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, nameStart, nameEnd, true, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, "_eq(", outCap, output, pos)
    pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, nameStart, nameEnd, true, outCap, output, pos)
    pos = Stigi.Emit.writeLiteral(4, " a, ", outCap, output, pos)
    pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, nameStart, nameEnd, true, outCap, output, pos)
    pos = Stigi.Emit.writeLine(5, " b) {", outCap, output, pos)
    pos = Stigi.Emit.writeLine(35, "  if (a.tag != b.tag) return false;", outCap, output, pos)
    pos = Stigi.Emit.writeLine(18, "  switch (a.tag) {", outCap, output, pos)
    var caseIdx: U64 = 0
    var localCase: U64 = 0
    while caseIdx < caseCount do
      @<lessOrEqual(caseIdx, caseCount), lessOrEqual(localCase, caseIdx), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(caseCount - caseIdx)>
      val nextCaseIdx: U64 = caseIdx + 1
      if caseEnumUnit(caseIdx) == enumUnit then
        val fieldBase: U64 = caseFieldBase(caseIdx)
        val fieldLen: U64 = caseFieldCount(caseIdx)
        pos = Stigi.Emit.writeLiteral(9, "    case ", outCap, output, pos)
        pos = Stigi.Emit.writeUnsignedDecimal{region}(localCase, outCap, output, pos)
        if fieldLen > 0 && fieldLen <= fieldCount && fieldBase < fieldCount then
          pos = Stigi.Emit.writeLiteral(10, "U: return ", outCap, output, pos)
          var fieldPos: U64 = fieldBase
          var fieldStep: U64 = 0
          while fieldStep < fieldLen && fieldPos < fieldCount do
            @<lessOrEqual(fieldStep, fieldLen), lessOrEqual(fieldStep, fieldCount), lessOrEqual(fieldPos, fieldCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(fieldCount - fieldStep)>
            if fieldStep > 0 then
              pos = Stigi.Emit.writeLiteral(4, " && ", outCap, output, pos)
            else
              ()
            val typeStart: U64 = fieldTypeStart(fieldPos)
            val typeEnd: U64 = fieldTypeEnd(fieldPos)
            if typeStart <= typeEnd && typeEnd <= srcCap then
              if Stigi.Emit.isPrimitiveTypeSlice(srcCap, srcBytes, typeStart, typeEnd) then
                pos = Stigi.Emit.writeLiteral(5, "a.u.c", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(localCase, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(2, ".f", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(fieldStep, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(9, " == b.u.c", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(localCase, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(2, ".f", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(fieldStep, outCap, output, pos)
              else
                pos = Stigi.Emit.writeSliceMangled(srcCap, srcBytes, typeStart, typeEnd, true, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(9, "_eq(a.u.c", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(localCase, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(2, ".f", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(fieldStep, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(7, ", b.u.c", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(localCase, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(2, ".f", outCap, output, pos)
                pos = Stigi.Emit.writeUnsignedDecimal{region}(fieldStep, outCap, output, pos)
                pos = Stigi.Emit.writeLiteral(1, ")", outCap, output, pos)
            else
              pos = Stigi.Emit.writeLiteral(4, "true", outCap, output, pos)
            fieldPos = fieldPos + 1
            fieldStep = fieldStep + 1
          pos = Stigi.Emit.writeLine(1, ";", outCap, output, pos)
        else
          pos = Stigi.Emit.writeLine(15, "U: return true;", outCap, output, pos)
        localCase = localCase + 1
      else
        ()
      caseIdx = nextCaseIdx
    pos = Stigi.Emit.writeLine(26, "    default: return false;", outCap, output, pos)
    pos = Stigi.Emit.writeLine(3, "  }", outCap, output, pos)
    Stigi.Emit.writeLine(1, "}", outCap, output, pos)
  else
    writeOffset
