// prompt: emit val Filled(arr) = Array.tryFill[T]{reg}(len, init) else case FillFailed => ... with deterministic free.
def Stigi.Emit.emitTryFillBind {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), arrBeg: U64, arrFin: U64, rhsStart: U64, rhsEnd: U64, elsePos: U64, stmtEnd: U64, blockEnd: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, target: Stigi.Emit.TargetKind, fuel: U64) <below(0, outCap), lessOrEqual(arrBeg, arrFin), lessOrEqual(arrFin, srcCap), lessOrEqual(rhsStart, rhsEnd), lessOrEqual(rhsEnd, stmtEnd), lessOrEqual(elsePos, stmtEnd), lessOrEqual(stmtEnd, blockEnd), lessOrEqual(blockEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: Unit =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(retBeg, retFin, monoBeg, monoFin, isTest, _, _) = meta
    val TypeSpan(_, _, elemBeg, elemFin, afterArrType) = Stigi.Emit.parseTypeSpan(tokCap, toks, rhsStart, rhsEnd)
    var callPos: U64 = if afterArrType <= rhsEnd then afterArrType else rhsEnd
    var foundCall: Bool = false
    while callPos < rhsEnd && !foundCall do
      @<lessOrEqual(callPos, rhsEnd)>
      val Token(curKind, _, _, _, _) = toks(callPos)
      if curKind == TokLeftParen then foundCall = true else callPos = callPos + 1
    if foundCall && callPos < rhsEnd then
      val Filled(argBegs) = Array.tryFill[U64]{region}(32, 0) else
        case FillFailed => ()
      val Filled(argFins) = Array.tryFill[U64]{region}(32, 0) else
        case FillFailed => ()
      val argCount: U64 = Stigi.Emit.splitArgs(tokCap, toks, callPos, rhsEnd, argBegs, argFins)
      val argZeroBeg: U64 = argBegs(0)
      val argZeroFin: U64 = argFins(0)
      val argOneBeg: U64 = argBegs(1)
      val argOneFin: U64 = argFins(1)
      if argCount >= 2 && argZeroBeg <= argZeroFin && argZeroFin <= rhsEnd && argOneBeg <= argOneFin && argOneFin <= rhsEnd && elemFin <= srcCap then
        val tid: U64 = writer.nextTemp()
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 16, "  uint64_t _alen")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, " = (uint64_t)(")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argZeroBeg, argZeroFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, elemBeg, elemFin, 0, 0, 0, 0)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " _iv")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, argOneBeg, argOneFin, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, elemBeg, elemFin, 0, 0, 0, 0)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, " *_arr")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " = (")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, elemBeg, elemFin, 0, 0, 0, 0)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 27, " *)stigi_rt_try_alloc(_alen")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, ", sizeof(")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, elemBeg, elemFin, 0, 0, 0, 0)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "));")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  if (_arr")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 11, " != NULL) {")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 21, "    for (uint64_t _fi")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, " = 0; _fi")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, " < _alen")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, "; _fi")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "++) _arr")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "[_fi")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "] = _iv")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "    ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, elemBeg, elemFin, 0, 0, 0, 0)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " *v_")
        writer.putSlice(outCap, outBuf, srcCap, src, arrBeg, arrFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, " = _arr")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 12, "    (void)v_")
        writer.putSlice(outCap, outBuf, srcCap, src, arrBeg, arrFin)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        var nextLocs: U64 = localCount
        if nextLocs < localCap then
          locals(nextLocs) := FieldInfo(FieldLocalArray, arrBeg, arrFin, 0, 0, elemBeg, elemFin)
          nextLocs = nextLocs + 1
        else ()
        val isVoidRet: Bool = if !isTest && retFin <= srcCap then Stigi.Lex.sliceEquals(srcCap, src, retBeg, retFin, 4, "Unit") else false
        val isRetTarget: Bool = target match
          case TargetReturn => true
          case TargetAssign(_) => false
          case TargetDiscard => false
        if isRetTarget && !isVoidRet then
          val resTid: U64 = writer.nextTemp()
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "    ")
          if isTest then Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "int32_t") else if retFin <= srcCap && monoFin <= srcCap then Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, monoBeg, monoFin) else Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint64_t")
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " _e")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, resTid)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 7, " = {0};")
          Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, stmtEnd, blockEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, nextLocs, meta, TargetAssign(resTid), nextFuel)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 13, "    free(_arr")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 13, "    return _e")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, resTid)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        else
          Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, stmtEnd, blockEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, nextLocs, meta, target, nextFuel)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 13, "    free(_arr")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 10, "  } else {")
        var failPos: U64 = elsePos
        var foundArrow: Bool = false
        while failPos < stmtEnd && !foundArrow do
          @<lessOrEqual(elsePos, failPos), lessOrEqual(failPos, stmtEnd)>
          val Token(failKind, _, _, _, _) = toks(failPos)
          failPos = failPos + 1
          if failKind == TokArrow then foundArrow = true else ()
        if foundArrow then
          Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, failPos, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, target, nextFuel)
        else ()
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "  }")
      else ()
    else ()
  else ()
