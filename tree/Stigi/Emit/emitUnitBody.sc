// prompt: iteratively lower a complete L0 def or Test body into ISO C11 with explicit block stack.
def Stigi.Emit.emitUnitBody {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), unitRetEnum: Array[U64](unitCount), callerStart: U64, callerSpaceEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCol: Array[U64](tokCap), tokCount: U64, bodyTok: U64, retTypeTok: U64, hasRetTok: Bool, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap), paramCount: U64, caseCap: U64, caseEnumUnit: Array[U64](caseCap), caseNameStart: Array[U64](caseCap), caseNameEnd: Array[U64](caseCap), caseFieldBase: Array[U64](caseCap), caseFieldCount: Array[U64](caseCap), caseCount: U64, fieldCap: U64, fieldTypeStart: Array[U64](fieldCap), fieldTypeEnd: Array[U64](fieldCap), fieldCount: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(unitCount, 4096), lessOrEqual(callerStart, callerSpaceEnd), lessOrEqual(callerSpaceEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(bodyTok, tokCount), below(srcCap, 0x100000000), lessOrEqual(paramCount, localCap), below(localCap, 256), lessOrEqual(caseCount, caseCap), below(caseCap, 4096), lessOrEqual(fieldCount, fieldCap), below(fieldCap, 4096), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val isVoidRet: Bool = if hasRetTok && retTypeTok < tokCount then Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, retTypeTok, 4, "Unit") else true
  Array.tryFill{region}(32, zeroByte) match
    case FillFailed => writeOffset
    case Filled(blkKinds) =>
      Array.tryFill{region}(32, zeroWord) match
        case FillFailed => writeOffset
        case Filled(blkCols) =>
          Array.tryFill{region}(32, zeroByte) match
            case FillFailed => writeOffset
            case Filled(blkFlags) =>
              Array.tryFill{region}(32, zeroWord) match
                case FillFailed => writeOffset
                case Filled(blkSavedLocals) =>
                  Array.tryFill{region}(32, zeroWord) match
                    case FillFailed => writeOffset
                    case Filled(blkTempNums) =>
                      Array.tryFill{region}(32, zeroWord) match
                        case FillFailed => writeOffset
                        case Filled(blkAuxStart) =>
                          Array.tryFill{region}(32, zeroWord) match
                            case FillFailed => writeOffset
                            case Filled(blkAuxEnd) =>
                              Array.tryFill{region}(32, zeroWord) match
                                case FillFailed => writeOffset
                                case Filled(blkPrev) =>
                                  Array.tryFill{region}(1, zeroWord) match
                                    case FillFailed => writeOffset
                                    case Filled(blkTopBox) =>
                                      Array.tryFill{region}(1, zeroWord) match
                                        case FillFailed => writeOffset
                                        case Filled(tempBox) =>
                                          Array.tryFill{region}(1, zeroWord) match
                                            case FillFailed => writeOffset
                                            case Filled(localCountBox) =>
                                              Array.tryFill{region}(4, zeroWord) match
                                                case FillFailed => writeOffset
                                                case Filled(targetBox) =>
                                                  blkTopBox(0) := 32
                                                  localCountBox(0) := paramCount
                                                  Stigi.Emit.pushBlockFrame(blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, 1, 0, if isVoidRet then 0 else 1, paramCount, 0, 0, 0)
                                                  var pos: U64 = writeOffset
                                                  var cursor: U64 = bodyTok
                                                  var stepBudget: U64 = 0
                                                  while cursor < tokCount && stepBudget < tokCount do
                                                    @<lessOrEqual(cursor, tokCount), lessOrEqual(stepBudget, tokCount), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(tokCount - stepBudget)>
                                                    if tokKind(cursor) == 7 then
                                                      cursor = cursor + 1
                                                    else
                                                      val lineEnd: U64 = Stigi.Emit.findLineEnd(tokCap, tokKind, tokCount, cursor)
                                                      val lineCol: U64 = tokCol(cursor)
                                                      val isElseLine: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "else")
                                                      val isCaseLine: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 4, "case")
                                                      pos = Stigi.Emit.closeBlocksToCol{region}(blkKinds, blkCols, blkSavedLocals, blkTempNums, blkPrev, blkTopBox, localCountBox, targetBox, lineCol, isElseLine || isCaseLine, false, outCap, output, pos)
                                                      if cursor < lineEnd then
                                                        pos = Stigi.Emit.emitBodyLine{region}(unitCount, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, callerStart, callerSpaceEnd, srcCap, srcBytes, tokCap, tokKind, tokStart, tokEnd, tokCol, tokCount, cursor, lineEnd, lineCol, retTypeTok, hasRetTok, isVoidRet, localCap, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, localCountBox, caseCap, caseEnumUnit, caseNameStart, caseNameEnd, caseFieldBase, caseFieldCount, caseCount, fieldCap, fieldTypeStart, fieldTypeEnd, fieldCount, blkKinds, blkCols, blkFlags, blkSavedLocals, blkTempNums, blkAuxStart, blkAuxEnd, blkPrev, blkTopBox, tempBox, targetBox, outCap, output, pos)
                                                      else
                                                        ()
                                                      cursor = if lineEnd < tokCount then lineEnd + 1 else tokCount
                                                    stepBudget = stepBudget + 1
                                                  pos = Stigi.Emit.closeBlocksToCol{region}(blkKinds, blkCols, blkSavedLocals, blkTempNums, blkPrev, blkTopBox, localCountBox, targetBox, 0, false, true, outCap, output, pos)
                                                  if !isVoidRet then
                                                    pos = Stigi.Emit.writeLiteral(8, "return (", outCap, output, pos)
                                                    if hasRetTok && retTypeTok < tokCount then
                                                      pos = Stigi.Emit.emitTypeName(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, retTypeTok, outCap, output, pos)
                                                    else
                                                      pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
                                                    pos = Stigi.Emit.writeLine(5, "){0};", outCap, output, pos)
                                                  else
                                                    ()
                                                  Stigi.Emit.writeLine(1, "}", outCap, output, pos)
