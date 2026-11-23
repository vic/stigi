// prompt: lower any flat or nested L0 expression token range to portable ISO C11 without recursion.
pub def Stigi.Emit.emitExprRange {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, exprStart: U64, exprEnd: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), localCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseCount: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprStart, exprEnd), lessOrEqual(exprEnd, tokCount), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val wrapTopOr: Bool = Stigi.Emit.needsTopOrWrap(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, exprStart, exprEnd)
  Array.tryFill{region}(32, zeroByte) match
    case FillFailed => writeOffset
    case Filled(frameKinds) =>
      Array.tryFill{region}(32, zeroByte) match
        case FillFailed => writeOffset
        case Filled(frameSubs) =>
          Array.tryFill{region}(32, zeroByte) match
            case FillFailed => writeOffset
            case Filled(frameArgs) =>
              Array.tryFill{region}(32, zeroWord) match
                case FillFailed => writeOffset
                case Filled(frameTargets) =>
                  Array.tryFill{region}(32, zeroWord) match
                    case FillFailed => writeOffset
                    case Filled(framePrev) =>
                      Array.tryFill{region}(1, zeroWord) match
                        case FillFailed => writeOffset
                        case Filled(topBox) =>
                          Array.tryFill{region}(1, zeroWord) match
                            case FillFailed => writeOffset
                            case Filled(cursorBox) =>
                              topBox(0) := 32
                              var pos: U64 = writeOffset
                              if wrapTopOr then
                                pos = Stigi.Emit.writeLiteral(1, "(", outCap, output, writeOffset)
                              else
                                ()
                              var cursor: U64 = exprStart
                              val maxSteps: U64 = tokCount + tokCount
                              var stepBudget: U64 = 0
                              while cursor < exprEnd && stepBudget < maxSteps do
                                @<lessOrEqual(exprStart, cursor), lessOrEqual(cursor, exprEnd), lessOrEqual(stepBudget, maxSteps), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(maxSteps - stepBudget)>
                                val preTop: U64 = topBox(0)
                                if preTop < 32 then
                                  if frameKinds(preTop) == 23 && frameTargets(preTop) == cursor then
                                    pos = Stigi.Emit.writeLiteral(3, ")))", outCap, output, pos)
                                    topBox(0) := framePrev(preTop)
                                  else
                                    ()
                                else
                                  ()
                                val midTop: U64 = topBox(0)
                                var handledEqualMid: Bool = false
                                if midTop < 32 then
                                  if frameKinds(midTop) == 22 && frameTargets(midTop) == cursor then
                                    val rhsStart: U64 = cursor + 1
                                    val rhsEnd: U64 = if rhsStart < exprEnd then Stigi.Emit.scanPrimaryEnd(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, rhsStart, exprEnd) else exprEnd
                                    pos = Stigi.Emit.writeLiteral(4, "), (", outCap, output, pos)
                                    frameKinds(midTop) := 23
                                    frameTargets(midTop) := rhsEnd
                                    cursor = rhsStart
                                    handledEqualMid = true
                                  else
                                    ()
                                else
                                  ()
                                if cursor < exprEnd && !handledEqualMid then
                                  val kindByte: U8 = tokKind(cursor)
                                  if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 2, "if") then
                                    pos = Stigi.Emit.writeLiteral(2, "((", outCap, output, pos)
                                    Stigi.Emit.pushExprFrame(frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, 18, 0, 0)
                                    cursor = cursor + 1
                                  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "then") then
                                    pos = Stigi.Emit.writeLiteral(5, ") ? (", outCap, output, pos)
                                    val curTop: U64 = topBox(0)
                                    if curTop < 32 then
                                      if frameKinds(curTop) == 18 then frameKinds(curTop) := 19 else ()
                                    else
                                      ()
                                    cursor = cursor + 1
                                  else if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "else") then
                                    pos = Stigi.Emit.popInlineElseFrames(frameKinds, framePrev, topBox, outCap, output, pos)
                                    pos = Stigi.Emit.writeLiteral(5, ") : (", outCap, output, pos)
                                    val curTop: U64 = topBox(0)
                                    if curTop < 32 then
                                      if frameKinds(curTop) == 19 then frameKinds(curTop) := 20 else ()
                                    else
                                      ()
                                    cursor = cursor + 1
                                  else if kindByte == 1 || kindByte == 2 then
                                    val beforeTop: U64 = topBox(0)
                                    pos = Stigi.Emit.tryStartEnumEqual(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, cursor, exprEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, outCap, output, pos)
                                    if topBox(0) == beforeTop then
                                      val nextPos: U64 = Stigi.Emit.emitExprIdentToken{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, cursor, exprEnd, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCount, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseCount, frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, cursorBox, outCap, output, pos)
                                      if nextPos >= writeOffset && nextPos <= outCap then pos = nextPos else ()
                                      val nextCur: U64 = cursorBox(0)
                                      cursor = if nextCur >= exprStart && nextCur <= exprEnd then nextCur else cursor + 1
                                    else
                                      ()
                                  else
                                    pos = Stigi.Emit.emitExprOtherToken(srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCount, cursor, exprEnd, frameKinds, frameSubs, frameArgs, frameTargets, framePrev, topBox, cursorBox, wrapTopOr, outCap, output, pos)
                                    val nextCur: U64 = cursorBox(0)
                                    cursor = if nextCur >= exprStart && nextCur <= exprEnd then nextCur else cursor + 1
                                else
                                  ()
                                stepBudget = stepBudget + 1
                              var drainStep: U64 = 0
                              var draining: Bool = true
                              while drainStep < 32 && draining do
                                @<lessOrEqual(drainStep, 32), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(32 - drainStep)>
                                val remTop: U64 = topBox(0)
                                if remTop < 32 then
                                  val topKind: U8 = frameKinds(remTop)
                                  if topKind == 23 then
                                    pos = Stigi.Emit.writeLiteral(3, ")))", outCap, output, pos)
                                    topBox(0) := framePrev(remTop)
                                  else if topKind == 20 then
                                    pos = Stigi.Emit.writeLiteral(2, "))", outCap, output, pos)
                                    topBox(0) := framePrev(remTop)
                                  else
                                    draining = false
                                else
                                  draining = false
                                drainStep = drainStep + 1
                              if wrapTopOr then Stigi.Emit.writeLiteral(1, ")", outCap, output, pos) else pos
