// prompt: emit one identifier or call-head token inside an L0 expression and update the cursor and stack.
def Stigi.Emit.emitExprIdentToken {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, cursor: U64, exprEnd: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, frameKinds: Array[U8](32), frameSubs: Array[U8](32), frameArgs: Array[U8](32), frameTargets: Array[U64](32), framePrev: Array[U64](32), topBox: Array[U64](1), cursorBox: Array[U64](1), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprEnd, tokCount), below(cursor, exprEnd), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 =
  val tokBeg: U64 = tokStart(cursor)
  val tokFin: U64 = tokEnd(cursor)
  val nextTok: U64 = cursor + 1
  cursorBox(0) := nextTok
  if tokBeg <= tokFin && tokFin <= srcCap then
    if Stigi.Parse.sliceEquals(srcCap, srcBytes, tokBeg, tokFin, 4, "true") || Stigi.Parse.sliceEquals(srcCap, srcBytes, tokBeg, tokFin, 5, "false") then
      Stigi.Print.writeSlice(srcCap, srcBytes, tokBeg, tokFin, outCap, output, writeOffset)
    else if Stigi.Parse.sliceEquals(srcCap, srcBytes, tokBeg, tokFin, 17, "Stigi.Raw.nullPtr") then
      Stigi.Emit.writeLiteral(4, "NULL", outCap, output, writeOffset)
    else
      Stigi.Emit.classifyBuiltinCall(srcCap, srcBytes, tokBeg, tokFin) match
        case BuiltinInfo(builtKind, builtSub) =>
          if builtKind != 0 then
            val argOpen: U64 = Stigi.Emit.stepOptionalBrace(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextTok, exprEnd)
            if argOpen < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, argOpen, 1, "(") then
              val argZeroStart: U64 = argOpen + 1
              cursorBox(0) := argZeroStart
              if builtKind == 15 && argZeroStart < exprEnd then
                val argZeroEnd: U64 = Stigi.Emit.scanPrimaryEnd(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, argZeroStart, exprEnd)
                val expectEnum: U64 = if argZeroEnd < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, argZeroEnd, 1, ",") then Stigi.Emit.detectPrimaryEnumUnit(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, argZeroStart, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) else unitCount
                if expectEnum < unitCount then
                  val enumBeg: U64 = nameStarts(expectEnum)
                  val enumFin: U64 = nameEnds(expectEnum)
                  var pos: U64 = Stigi.Emit.writeLiteral(1, "(", outCap, output, writeOffset)
                  if enumBeg <= enumFin && enumFin <= indexCap then
                    pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, enumBeg, enumFin, true, outCap, output, pos)
                  else
                    ()
                  pos = Stigi.Emit.writeLiteral(5, "_eq((", outCap, output, pos)
                  Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 16, 0, 0)
                  pos
                else
                  Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 15, 0, 0)
                  Stigi.Emit.emitBuiltinOpen(15, 0, outCap, output, writeOffset)
              else
                Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, builtKind, builtSub, 0)
                Stigi.Emit.emitBuiltinOpen(builtKind, builtSub, outCap, output, writeOffset)
            else
              Stigi.Print.writeSlice(srcCap, srcBytes, tokBeg, tokFin, outCap, output, writeOffset)
          else
            val locIdx: U64 = Stigi.Emit.findLocalIndex(srcCap, srcBytes, tokBeg, tokFin, localCap, localNameStart, localNameEnd, localCount)
            if locIdx < localCount then
              var pos: U64 = Stigi.Print.writeSlice(srcCap, srcBytes, tokBeg, tokFin, outCap, output, writeOffset)
              if localIsArray(locIdx) == 1 && nextTok < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextTok, 1, "(") then
                pos = Stigi.Emit.writeLiteral(1, "[", outCap, output, pos)
                Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 2, 0, 0)
                cursorBox(0) := nextTok + 1
                pos
              else
                pos
            else
              val unitHit: U64 = Stigi.Emit.findUnitByName(unitCount, indexCap, indexBytes, nameStarts, nameEnds, srcCap, srcBytes, tokBeg, tokFin)
              if unitHit < unitCount then
                if kindCodes(unitHit) == 4 then
                  cursorBox(0) := Stigi.Emit.scanPrimaryEnd(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, exprEnd)
                  Stigi.Emit.writeLiteral(7, "(void)0", outCap, output, writeOffset)
                else
                  val unitBeg: U64 = nameStarts(unitHit)
                  val unitFin: U64 = nameEnds(unitHit)
                  var pos: U64 = if unitBeg <= unitFin && unitFin <= indexCap then Stigi.Emit.writeSliceMangled(indexCap, indexBytes, unitBeg, unitFin, true, outCap, output, writeOffset) else writeOffset
                  if kindCodes(unitHit) == 5 then
                    pos
                  else
                    val argOpen: U64 = Stigi.Emit.stepOptionalBrace(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextTok, exprEnd)
                    if argOpen < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, argOpen, 1, "(") then
                      pos = Stigi.Emit.writeLiteral(1, "(", outCap, output, pos)
                      Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 1, 0, 0)
                      cursorBox(0) := argOpen + 1
                      pos
                    else
                      cursorBox(0) := argOpen
                      pos
              else
                Stigi.Emit.findEnumCase(unitCount, indexCap, indexBytes, nameStarts, nameEnds, callerStart, callerSpaceEnd, srcCap, srcBytes, tokBeg, tokFin, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount) match
                  case CaseHit(caseFound, _, enumHit, localTag) =>
                    if caseFound && enumHit < unitCount then
                      val enumBeg: U64 = nameStarts(enumHit)
                      val enumFin: U64 = nameEnds(enumHit)
                      if nextTok < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextTok, 1, "(") then
                        val pos: U64 = if enumBeg <= enumFin && enumFin <= indexCap then Stigi.Emit.emitEnumCtorOpen{region}(indexCap, indexBytes, enumBeg, enumFin, localTag, true, outCap, output, writeOffset) else writeOffset
                        Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 14, 0, 0)
                        cursorBox(0) := nextTok + 1
                        pos
                      else
                        if enumBeg <= enumFin && enumFin <= indexCap then Stigi.Emit.emitEnumCtorOpen{region}(indexCap, indexBytes, enumBeg, enumFin, localTag, false, outCap, output, writeOffset) else writeOffset
                    else
                      Stigi.Print.writeSlice(srcCap, srcBytes, tokBeg, tokFin, outCap, output, writeOffset)
  else
    writeOffset
