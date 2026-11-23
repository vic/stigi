// prompt: execute passes one through four of self-hosted ISO C11 emission over a sorted unit image.
def Stigi.Emit.runCompilePasses {region: Stigi.Region, Stigi.Mut} (unitCount: U64, order: Array[U64](unitCount), indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), srcCap: U64, srcBytes: Array[U8](srcCap), unitSrcStart: Array[U64](unitCount), unitSrcEnd: Array[U64](unitCount), tokKind: Array[U8](8192), tokStart: Array[U64](8192), tokEnd: Array[U64](8192), tokLine: Array[U64](8192), tokCol: Array[U64](8192), caseEnumUnit: Array[U64](1024), caseNameStart: Array[U64](1024), caseNameEnd: Array[U64](1024), caseFieldBase: Array[U64](1024), caseFieldCount: Array[U64](1024), fieldNameStart: Array[U64](2048), fieldNameEnd: Array[U64](2048), fieldTypeStart: Array[U64](2048), fieldTypeEnd: Array[U64](2048), localNameStart: Array[U64](128), localNameEnd: Array[U64](128), localTypeStart: Array[U64](128), localTypeEnd: Array[U64](128), localIsArray: Array[U8](128), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), below(indexCap, 0x100000000), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val zeroWord: U64 = 0
  Array.tryFill{region}(3, zeroWord) match
    case FillFailed => writeOffset
    case Filled(stateBox) =>
      var hasMain: Bool = false
      var slotOne: U64 = 0
      while slotOne < unitCount do
        @<lessOrEqual(slotOne, unitCount), decreases(unitCount - slotOne)>
        val unitIdx: U64 = order(slotOne)
        if unitIdx < unitCount then
          unitRetEnum(unitIdx) := unitCount
          val nameBeg: U64 = nameStarts(unitIdx)
          val nameFin: U64 = nameEnds(unitIdx)
          if nameBeg <= nameFin && nameFin <= indexCap then
            if Stigi.Parse.sliceEquals(indexCap, indexBytes, nameBeg, nameFin, 4, "main") then
              hasMain = true
            else
              ()
          else
            ()
          val srcBeg: U64 = unitSrcStart(unitIdx)
          val srcFin: U64 = unitSrcEnd(unitIdx)
          if srcBeg <= srcFin && srcFin <= srcCap then
            val tokCount: U64 = Stigi.Emit.tokenizeSource(srcCap, srcBytes, srcBeg, srcFin, 8192, tokKind, tokStart, tokEnd, tokLine, tokCol)
            val curCases: U64 = stateBox(0)
            val curFields: U64 = stateBox(1)
            if kindCodes(unitIdx) == 1 && curCases <= 1024 && curFields <= 2048 then
              Stigi.Emit.registerEnumUnit(unitIdx, srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount, 1024, caseEnumUnit, caseNameStart, caseNameEnd, caseFieldBase, caseFieldCount, curCases, 2048, fieldNameStart, fieldNameEnd, fieldTypeStart, fieldTypeEnd, curFields) match
                case EnumTotals(nextCases, nextFields) =>
                  stateBox(0) := nextCases
                  stateBox(1) := nextFields
            else if kindCodes(unitIdx) == 3 then
              Stigi.Emit.scanDefSignature(srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount) match
                case DefSignature(_, _, retTok, hasRet, _) =>
                  if hasRet && retTok < tokCount then
                    val retBeg: U64 = tokStart(retTok)
                    val retFin: U64 = tokEnd(retTok)
                    if retBeg <= retFin && retFin <= srcCap then
                      val retUnit: U64 = Stigi.Emit.findUnitByName(unitCount, indexCap, indexBytes, nameStarts, nameEnds, srcCap, srcBytes, retBeg, retFin)
                      if retUnit < unitCount && kindCodes(retUnit) == 1 then unitRetEnum(unitIdx) := retUnit else ()
                    else
                      ()
                  else
                    ()
            else
              ()
          else
            ()
        else
          ()
        slotOne = slotOne + 1
      val rawCases: U64 = stateBox(0)
      val rawFields: U64 = stateBox(1)
      val caseTotal: U64 = if rawCases <= 1024 then rawCases else 1024
      val fieldTotal: U64 = if rawFields <= 2048 then rawFields else 2048
      var pos: U64 = Stigi.Emit.writeRuntimePreamble(outCap, output, writeOffset)
      pos = Stigi.Emit.emitAllEnumsOrdered{region}(unitCount, order, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, srcCap, srcBytes, 1024, caseEnumUnit, caseFieldBase, caseFieldCount, caseTotal, 2048, fieldTypeStart, fieldTypeEnd, fieldTotal, outCap, output, pos)
      stateBox(2) := pos
      var slotTwo: U64 = 0
      while slotTwo < unitCount do
        @<lessOrEqual(slotTwo, unitCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(unitCount - slotTwo)>
        val unitIdx: U64 = order(slotTwo)
        if unitIdx < unitCount then
          val unitKind: U8 = kindCodes(unitIdx)
          val isPassTwoKind: Bool = unitKind == 5 || unitKind == 3 || unitKind == 11
          val nameBeg: U64 = nameStarts(unitIdx)
          val nameFin: U64 = nameEnds(unitIdx)
          val srcBeg: U64 = unitSrcStart(unitIdx)
          val srcFin: U64 = unitSrcEnd(unitIdx)
          if isPassTwoKind && nameBeg <= nameFin && nameFin <= indexCap && srcBeg <= srcFin && srcFin <= srcCap then
            val tokCount: U64 = Stigi.Emit.tokenizeSource(srcCap, srcBytes, srcBeg, srcFin, 8192, tokKind, tokStart, tokEnd, tokLine, tokCol)
            val basePos: U64 = pos
            Stigi.Emit.scanDefSignature(srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount) match
              case DefSignature(paramListStart, afterParams, retTok, hasRet, bodyTok) =>
                if unitKind == 5 then
                  stateBox(2) := Stigi.Emit.emitTopLevelVal(indexCap, indexBytes, nameBeg, nameFin, srcCap, srcBytes, 8192, tokKind, tokStart, tokEnd, tokCount, retTok, hasRet, bodyTok, outCap, output, basePos)
                else if bodyTok < tokCount && paramListStart <= afterParams && afterParams <= tokCount then
                  stateBox(2) := Stigi.Emit.emitFunctionHeader(indexCap, indexBytes, nameBeg, nameFin, srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount, paramListStart, afterParams, retTok, hasRet, true, outCap, output, basePos)
                else
                  ()
            val updatedPos: U64 = stateBox(2)
            if updatedPos >= writeOffset && updatedPos <= outCap then pos = updatedPos else ()
          else
            ()
        else
          ()
        slotTwo = slotTwo + 1
      var slotThree: U64 = 0
      while slotThree < unitCount do
        @<lessOrEqual(slotThree, unitCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(unitCount - slotThree)>
        val unitIdx: U64 = order(slotThree)
        if unitIdx < unitCount then
          val unitKind: U8 = kindCodes(unitIdx)
          val isPassThreeKind: Bool = unitKind == 3 || unitKind == 11
          val nameBeg: U64 = nameStarts(unitIdx)
          val nameFin: U64 = nameEnds(unitIdx)
          val srcBeg: U64 = unitSrcStart(unitIdx)
          val srcFin: U64 = unitSrcEnd(unitIdx)
          if isPassThreeKind && nameBeg <= nameFin && nameFin <= indexCap && srcBeg <= srcFin && srcFin <= srcCap then
            val tokCount: U64 = Stigi.Emit.tokenizeSource(srcCap, srcBytes, srcBeg, srcFin, 8192, tokKind, tokStart, tokEnd, tokLine, tokCol)
            val basePos: U64 = pos
            Stigi.Emit.scanDefSignature(srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount) match
              case DefSignature(paramListStart, afterParams, retTok, hasRet, bodyTok) =>
                if bodyTok < tokCount && paramListStart <= afterParams && afterParams <= tokCount then
                  val posOne: U64 = Stigi.Emit.writeLiteral(9, "#line 1 \"", outCap, output, basePos)
                  val posTwo: U64 = Stigi.Print.writeSlice(indexCap, indexBytes, nameBeg, nameFin, outCap, output, posOne)
                  val posThree: U64 = Stigi.Emit.writeLine(4, ".sc\"", outCap, output, posTwo)
                  val posFour: U64 = Stigi.Emit.emitFunctionHeader(indexCap, indexBytes, nameBeg, nameFin, srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount, paramListStart, afterParams, retTok, hasRet, false, outCap, output, posThree)
                  val paramCount: U64 = Stigi.Emit.collectFunctionParams(srcCap, srcBytes, 8192, tokStart, tokEnd, tokCount, paramListStart, afterParams, 128, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray)
                  val callerSpaceEnd: U64 = Stigi.Emit.findCallerSpaceEnd(indexCap, indexBytes, nameBeg, nameFin)
                  stateBox(2) := Stigi.Emit.emitUnitBody{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, nameBeg, callerSpaceEnd, srcCap, srcBytes, 8192, tokKind, tokStart, tokEnd, tokCol, tokCount, bodyTok, retTok, hasRet, 128, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, paramCount, 1024, caseEnumUnit, caseNameStart, caseNameEnd, caseFieldBase, caseFieldCount, caseTotal, 2048, fieldTypeStart, fieldTypeEnd, fieldTotal, outCap, output, posFour)
                else
                  ()
            val updatedPos: U64 = stateBox(2)
            if updatedPos >= writeOffset && updatedPos <= outCap then pos = updatedPos else ()
          else
            ()
        else
          ()
        slotThree = slotThree + 1
      Stigi.Emit.writeRuntimeEntry(unitCount, order, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, hasMain, outCap, output, pos)
