// prompt: emit simple val/var bindings including string-literal arrays, wildcards, match, and if initializers.
def Stigi.Emit.emitSimpleValBind {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), namePos: U64, stmtEnd: U64, isMutable: Bool, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, meta: Stigi.Emit.BodyMeta, fuel: U64) <below(0, outCap), below(namePos, stmtEnd), lessOrEqual(stmtEnd, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(localCount, localCap), lessOrEqual(fuel, 64), decreases(fuel)>: U64 <lessOrEqual(result, localCap)> =
  if fuel > 0 then
    val nextFuel: U64 = fuel - 1
    val BodyMeta(_, _, monoBeg, monoFin, _, _, _) = meta
    val Token(_, nameBeg, nameFin, _, _) = toks(namePos)
    var assignPos: U64 = stmtEnd
    var typeBeg: U64 = 0
    var typeFin: U64 = 0
    var elemBeg: U64 = 0
    var elemFin: U64 = 0
    var scanPos: U64 = namePos + 1
    while scanPos < stmtEnd && assignPos == stmtEnd do
      @<lessOrEqual(namePos, scanPos), lessOrEqual(scanPos, stmtEnd), lessOrEqual(typeBeg, typeFin), lessOrEqual(elemBeg, elemFin)>
      val Token(curKind, _, _, _, _) = toks(scanPos)
      if curKind == TokColon && scanPos + 1 < stmtEnd then
        val TypeSpan(valTypeBeg, valTypeFin, valElemBeg, valElemFin, afterType) = Stigi.Emit.parseTypeSpan(tokCap, toks, scanPos + 1, stmtEnd)
        typeBeg = valTypeBeg
        typeFin = valTypeFin
        elemBeg = valElemBeg
        elemFin = valElemFin
        scanPos = if afterType > scanPos && afterType <= stmtEnd then afterType else scanPos + 1
      else if curKind == TokAssign then assignPos = scanPos
      else scanPos = scanPos + 1
    val rhsStart: U64 = if assignPos < stmtEnd then assignPos + 1 else stmtEnd
    val isWild: Bool = if nameFin <= srcCap then Stigi.Lex.sliceEquals(srcCap, src, nameBeg, nameFin, 1, "_") else false
    if isWild then
      if rhsStart < stmtEnd then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "  (void)(")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, rhsStart, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
      else ()
      localCount
    else
      var firstRhs: U64 = rhsStart
      var trimRhs: Bool = true
      while firstRhs < stmtEnd && trimRhs do
        @<lessOrEqual(rhsStart, firstRhs), lessOrEqual(firstRhs, stmtEnd)>
        val Token(rhsKind, _, _, _, _) = toks(firstRhs)
        if rhsKind == TokNewline then firstRhs = firstRhs + 1 else trimRhs = false
      var matchPos: U64 = stmtEnd
      var startsWithIf: Bool = false
      if firstRhs < stmtEnd then
        val Token(headKind, _, _, _, _) = toks(firstRhs)
        if headKind == TokIf then startsWithIf = true else ()
      else ()
      var checkPos: U64 = firstRhs
      var lineDone: Bool = false
      while checkPos < stmtEnd && !lineDone do
        @<lessOrEqual(firstRhs, checkPos), lessOrEqual(checkPos, stmtEnd), lessOrEqual(firstRhs, matchPos), lessOrEqual(matchPos, stmtEnd)>
        val Token(chkKind, _, _, _, _) = toks(checkPos)
        if chkKind == TokNewline then lineDone = true
        else if chkKind == TokMatch && matchPos == stmtEnd then matchPos = checkPos
        else ()
        checkPos = checkPos + 1
      val isStrArr: Bool = if firstRhs < stmtEnd && typeFin <= srcCap then
        val Token(firstRhsKind, _, _, _, _) = toks(firstRhs)
        firstRhsKind == TokStr && Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 5, "Array")
      else false
      var nextLocs: U64 = localCount
      val isBlockRhs: Bool = matchPos < stmtEnd || startsWithIf
      if firstRhs < stmtEnd && isStrArr && nameFin <= srcCap then
        val Token(_, strBeg, strFin, _, _) = toks(firstRhs)
        if strFin <= srcCap then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 12, "  uint8_t v_")
          writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, "[] = ")
          writer.putSlice(outCap, outBuf, srcCap, src, strBeg, strFin)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  (void)v_")
          writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        else ()
        if nextLocs < localCap then
          locals(nextLocs) := FieldInfo(FieldLocalArray, nameBeg, nameFin, typeBeg, typeFin, elemBeg, elemFin)
          nextLocs = nextLocs + 1
        else ()
      else if isBlockRhs && nameFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap && monoFin <= srcCap then
        val tid: U64 = writer.nextTemp()
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, monoBeg, monoFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " _e")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 7, " = {0};")
        if matchPos < stmtEnd then
          val armsBeg: U64 = matchPos + 1
          Stigi.Emit.emitMatch{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, firstRhs, matchPos, armsBeg, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, TargetAssign(tid), nextFuel)
        else
          Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, firstRhs, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, TargetAssign(tid), nextFuel)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, monoBeg, monoFin)
        if isMutable then Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
        else Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, " const v_")
        writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, " = _e")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tid)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  (void)v_")
        writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        if nextLocs < localCap then
          val isArr: Bool = Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 5, "Array")
          locals(nextLocs) := FieldInfo(if isArr then FieldLocalArray else FieldLocal, nameBeg, nameFin, typeBeg, typeFin, elemBeg, elemFin)
          nextLocs = nextLocs + 1
        else ()
      else if nameFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap && monoFin <= srcCap then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, monoBeg, monoFin)
        if isMutable then Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
        else Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, " const v_")
        writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
        Stigi.Emit.emitExpr{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, firstRhs, stmtEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, localCap, locals, localCount, meta, nextFuel)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  (void)v_")
        writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        if nextLocs < localCap then
          val isArr: Bool = Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 5, "Array")
          locals(nextLocs) := FieldInfo(if isArr then FieldLocalArray else FieldLocal, nameBeg, nameFin, typeBeg, typeFin, elemBeg, elemFin)
          nextLocs = nextLocs + 1
        else ()
      else ()
      nextLocs
  else localCount
