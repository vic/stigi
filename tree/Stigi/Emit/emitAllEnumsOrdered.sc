// prompt: emit all enum typedefs and equality helpers in deterministic dependency order.
pub def Stigi.Emit.emitAllEnumsOrdered {region: Stigi.Region, Stigi.Mut} (unitCount: U64, order: Array[U64](unitCount), indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), srcCap: U64, srcBytes: Array[U8](srcCap), caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), below(indexCap, 0x100000000), below(srcCap, 0x100000000), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val zeroByte: U8 = 0
  Array.tryFill{region}(unitCount, zeroByte) match
    case FillFailed => writeOffset
    case Filled(enumEmitted) =>
      var pos: U64 = writeOffset
      var passIdx: U64 = 0
      while passIdx < 8 do
        @<lessOrEqual(passIdx, 8), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(8 - passIdx)>
        var slotIdx: U64 = 0
        while slotIdx < unitCount do
          @<lessOrEqual(slotIdx, unitCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(unitCount - slotIdx)>
          val unitIdx: U64 = order(slotIdx)
          if unitIdx < unitCount && kindCodes(unitIdx) == 1 && enumEmitted(unitIdx) == 0 then
            if Stigi.Emit.enumDepsReady(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, enumEmitted, unitIdx, srcCap, srcBytes, caseCap, caseEnumUnit, caseFieldBase, caseFieldCount, caseCount, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount) then
              pos = Stigi.Emit.emitEnumTypedef{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, unitIdx, srcCap, srcBytes, caseCap, caseEnumUnit, caseFieldBase, caseFieldCount, caseCount, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, outCap, output, pos)
              pos = Stigi.Emit.emitEnumEqualHelper{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, unitIdx, srcCap, srcBytes, caseCap, caseEnumUnit, caseFieldBase, caseFieldCount, caseCount, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, outCap, output, pos)
              enumEmitted(unitIdx) := 1
            else
              ()
          else
            ()
          slotIdx = slotIdx + 1
        passIdx = passIdx + 1
      pos
