// prompt: emit the ISO C tagged-union typedef for one registered L0 enum unit.
def Stigi.Emit.emitEnumTypedef {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), enumUnit: U64, srcCap: U64, srcBytes: Array[U8](srcCap), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(enumUnit, unitCount), below(unitCount, 4096), below(indexCap, 0x100000000), below(srcCap, 0x100000000), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLine(16, "typedef struct {", outCap, output, writeOffset)
  pos = Stigi.Emit.writeLine(15, "  uint32_t tag;", outCap, output, pos)
  pos = Stigi.Emit.writeLine(9, "  union {", outCap, output, pos)
  pos = Stigi.Emit.writeLine(19, "    uint8_t _empty;", outCap, output, pos)
  var caseIdx: U64 = 0
  var localCase: U64 = 0
  while caseIdx < caseCount do
    @<lessOrEqual(caseIdx, caseCount), lessOrEqual(localCase, caseIdx), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(caseCount - caseIdx)>
    val nextCaseIdx: U64 = caseIdx + 1
    if caseEnumUnit(caseIdx) == enumUnit then
      val fieldBase: U64 = caseFieldBase(caseIdx)
      val fieldLen: U64 = caseFieldCount(caseIdx)
      if fieldLen > 0 && fieldLen <= fieldCount && fieldBase < fieldCount then
        pos = Stigi.Emit.writeLiteral(13, "    struct { ", outCap, output, pos)
        var fieldPos: U64 = fieldBase
        var fieldStep: U64 = 0
        while fieldStep < fieldLen && fieldPos < fieldCount do
          @<lessOrEqual(fieldStep, fieldLen), lessOrEqual(fieldStep, fieldCount), lessOrEqual(fieldPos, fieldCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(fieldCount - fieldStep)>
          val typeStart: U64 = fieldTypeStart(fieldPos)
          val typeEnd: U64 = fieldTypeEnd(fieldPos)
          if typeStart <= typeEnd && typeEnd <= srcCap then
            pos = Stigi.Emit.emitTypeSlice(srcCap, srcBytes, typeStart, typeEnd, outCap, output, pos)
          else
            pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
          pos = Stigi.Emit.writeLiteral(2, " f", outCap, output, pos)
          pos = Stigi.Emit.writeUnsignedDecimal{region}(fieldStep, outCap, output, pos)
          pos = Stigi.Emit.writeLiteral(2, "; ", outCap, output, pos)
          fieldPos = fieldPos + 1
          fieldStep = fieldStep + 1
        pos = Stigi.Emit.writeLiteral(3, "} c", outCap, output, pos)
        pos = Stigi.Emit.writeUnsignedDecimal{region}(localCase, outCap, output, pos)
        pos = Stigi.Emit.writeLine(1, ";", outCap, output, pos)
      else
        ()
      localCase = localCase + 1
    else
      ()
    caseIdx = nextCaseIdx
  pos = Stigi.Emit.writeLine(6, "  } u;", outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(2, "} ", outCap, output, pos)
  val nameStart: U64 = nameStarts(enumUnit)
  val nameEnd: U64 = nameEnds(enumUnit)
  if nameStart <= nameEnd && nameEnd <= indexCap then
    pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, nameStart, nameEnd, true, outCap, output, pos)
  else
    ()
  Stigi.Emit.writeLine(1, ";", outCap, output, pos)
