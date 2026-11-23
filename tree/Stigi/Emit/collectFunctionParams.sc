// prompt: collect parameter names, type slices, and array flags into flat local tables.
pub def Stigi.Emit.collectFunctionParams {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, paramListStart: U64, afterParams: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localTypeStart: Array[U64](localCap), localTypeEnd: Array[U64](localCap), localIsArray: Array[U8](localCap)) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(paramListStart, afterParams), lessOrEqual(afterParams, tokCount), below(srcCap, 0x100000000), below(localCap, 256)>: U64 <lessOrEqual(result, localCap)> =
  var paramCount: U64 = 0
  if paramListStart < afterParams then
    var cursor: U64 = paramListStart + 1
    var stepBudget: U64 = 0
    while cursor < afterParams && stepBudget < tokCount do
      @<lessOrEqual(cursor, tokCount), lessOrEqual(paramCount, localCap), lessOrEqual(stepBudget, tokCount), decreases(tokCount - stepBudget)>
      val colonTok: U64 = cursor + 1
      val typePos: U64 = cursor + 2
      if typePos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, colonTok, 1, ":") then
        val isArray: Bool = Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, typePos, 5, "Array")
        val elemPos: U64 = typePos + 2
        if paramCount < localCap then
          localNameStart(paramCount) := tokStart(cursor)
          localNameEnd(paramCount) := tokEnd(cursor)
          localIsArray(paramCount) := if isArray then 1 else 0
          if isArray && elemPos < afterParams then
            localTypeStart(paramCount) := tokStart(elemPos)
            localTypeEnd(paramCount) := tokEnd(elemPos)
          else
            localTypeStart(paramCount) := tokStart(typePos)
            localTypeEnd(paramCount) := tokEnd(typePos)
          paramCount = paramCount + 1
        else
          ()
        var nextPos: U64 = typePos + 1
        if nextPos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, "[") then
          nextPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextPos, 91, 93)
        else
          ()
        if nextPos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, "(") then
          nextPos = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, nextPos, 40, 41)
        else
          ()
        cursor = if nextPos < afterParams && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, nextPos, 1, ",") then nextPos + 1 else nextPos
      else
        cursor = cursor + 1
      stepBudget = stepBudget + 1
  else
    ()
  paramCount
