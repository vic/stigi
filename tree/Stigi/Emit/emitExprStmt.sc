// prompt: emit match, assignment, test expect, or tail expression statement cleanly from a statement slice.
def Stigi.Emit.emitExprStmt {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), pos: U64, stmtEnd: U64, hasMore: Bool, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), locCount: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), below(pos, stmtEnd), lessOrEqual(stmtEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(locCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(retBeg, retFin, _, _, isTest, _, _) = meta
    val Token(_, leadBeg, leadFin, _, _) = toks(pos)
    var matchPos: U64 = stmtEnd
    var colAssignPos: U64 = stmtEnd
    var plainAssignPos: U64 = stmtEnd
    var scanPos: U64 = pos
    var parenDepth: U64 = 0
    var seenLineEnd: Bool = false
    while scanPos < stmtEnd && !seenLineEnd do
      @<lessOrEqual(pos, scanPos), lessOrEqual(scanPos, stmtEnd), lessOrEqual(pos, matchPos), lessOrEqual(matchPos, stmtEnd), lessOrEqual(pos, colAssignPos), lessOrEqual(colAssignPos, stmtEnd), lessOrEqual(pos, plainAssignPos), lessOrEqual(plainAssignPos, stmtEnd), lessOrEqual(parenDepth, Stigi.Limit.maxTokens)>
      val Token(scanKind, _, _, _, _) = toks(scanPos)
      if scanKind == TokNewline && parenDepth == 0 then seenLineEnd = true else if scanKind == TokLeftParen && parenDepth < Stigi.Limit.maxTokens then parenDepth = parenDepth + 1 else if scanKind == TokRightParen && parenDepth > 0 then parenDepth = parenDepth - 1 else if parenDepth == 0 then
        if scanKind == TokMatch && matchPos == stmtEnd then matchPos = scanPos else if scanKind == TokColonAssign && colAssignPos == stmtEnd then colAssignPos = scanPos else if scanKind == TokAssign && plainAssignPos == stmtEnd then plainAssignPos = scanPos else ()
      else ()
      scanPos = scanPos + 1
    if matchPos < stmtEnd then
      val armsBeg: U64 = matchPos + 1
      val matchTarget: Stigi.Emit.TargetKind = if hasMore then TargetDiscard else target
      Stigi.Emit.emitMatch{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, matchPos, armsBeg, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, matchTarget, nextFuel)
    else if colAssignPos < stmtEnd then
      val rhsBeg: U64 = colAssignPos + 1
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, colAssignPos, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
      Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsBeg, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
    else if plainAssignPos < stmtEnd then
      val rhsBeg: U64 = plainAssignPos + 1
      if leadFin <= srcCap then
        var isStateField: Bool = false
        var locIdx: U64 = locCount
        var foundLoc: Bool = false
        while locIdx > 0 && !foundLoc do
          @<lessOrEqual(locIdx, locCount)>
          locIdx = locIdx - 1
          val FieldInfo(locKind, locBeg, locFin, _, _, _, _) = locals(locIdx)
          if locFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, leadBeg, leadFin, locBeg, locFin) then
            foundLoc = true
            isStateField = locKind == FieldStateVar
          else ()
        if isStateField then Stigi.Emit.writeLit{writer}(outCap, outBuf, 13, "  _hstate->f_") else Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "  v_")
        writer.putSlice(outCap, outBuf, srcCap, src, leadBeg, leadFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsBeg, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
      else ()
    else
      val TypeSpan(callBeg, callFin, _, _, afterCall) = Stigi.Emit.parseTypeSpan(tokCap, toks, pos, stmtEnd)
      val isTestExpect: Bool = if callFin <= srcCap then Stigi.Lex.sliceEquals(srcCap, src, callBeg, callFin, 17, "Stigi.Test.expect") else false
      if isTestExpect && afterCall < stmtEnd then
        Stigi.Emit.emitTestExpect{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, afterCall, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, target, nextFuel)
      else if hasMore then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "  (void)(")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
      else
        val isVoidRet: Bool = if !isTest && retFin <= srcCap then Stigi.Lex.sliceEquals(srcCap, src, retBeg, retFin, 4, "Unit") else false
        target match
          case TargetReturn =>
            if isVoidRet then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "  (void)(")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
              Stigi.Emit.writeLine{writer}(outCap, outBuf, 10, "); return;")
            else
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  return (")
              Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
              Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
          case TargetAssign(tempNum) =>
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "  _e")
            Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tempNum)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " = (")
            Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
          case TargetDiscard =>
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "  (void)(")
            Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, pos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, locCount, meta, nextFuel)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
  else ()
