// prompt: emit C11 local variable bindings for pattern payload fields and register them in locals.
def Stigi.Emit.emitPatBinds {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), patBeg: U64, patFin: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), initLocals: U64, scrutTemp: U64, paramBeg: U64, paramFin: U64) <below(0, outCap), lessOrEqual(patBeg, patFin), lessOrEqual(patFin, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), lessOrEqual(initLocals, localCap), lessOrEqual(paramBeg, paramFin), lessOrEqual(paramFin, srcCap)>: U64 <lessOrEqual(result, localCap)> =
  var locCount: U64 = initLocals
  if patBeg < patFin then
    val Token(kind, caseNameBeg, caseNameFin, _, _) = toks(patBeg)
    if kind == TokUpper && caseNameFin <= srcCap && patBeg + 1 < patFin then
      val nextPos: U64 = patBeg + 1
      val Token(openKind, _, _, _, _) = toks(nextPos)
      if openKind == TokLeftParen then
        var foundCase: Bool = false
        var subStart: U64 = 0
        var subCount: U64 = 0
        var symIdx: U64 = 0
        while symIdx < symCount && !foundCase do
          @<lessOrEqual(symIdx, symCount), lessOrEqual(subStart, fieldCount), lessOrEqual(subCount, fieldCount)>
          val SymEntry(symKind, _, symBeg, symFin, _, _, fldBegIdx, fldCnt, _, _) = syms(symIdx)
          if symKind == SymCase && symFin <= srcCap && fldBegIdx <= fieldCount && fldCnt <= fieldCount && Stigi.Emit.sliceEqual(srcCap, src, caseNameBeg, caseNameFin, symBeg, symFin) then
            foundCase = true
            subStart = fldBegIdx
            subCount = fldCnt
          else ()
          symIdx = symIdx + 1
        if foundCase then
          var scanPos: U64 = nextPos + 1
          var fldOff: U64 = 0
          var doneBinds: Bool = false
          while scanPos < patFin && !doneBinds do
            @<lessOrEqual(patBeg, scanPos), lessOrEqual(scanPos, patFin), lessOrEqual(fldOff, subCount), lessOrEqual(locCount, localCap)>
            val Token(bindKind, bindBeg, bindFin, _, _) = toks(scanPos)
            if bindKind == TokRightParen then doneBinds = true else if bindKind == TokComma then scanPos = scanPos + 1 else
              if fldOff < subCount && bindFin <= srcCap then
                val isWild: Bool = Stigi.Lex.sliceEquals(srcCap, src, bindBeg, bindFin, 1, "_")
                val fldPos: U64 = subStart + fldOff
                if !isWild && fldPos < fieldCount then
                  val FieldInfo(_, fldBeg, fldFin, typeBeg, typeFin, elemBeg, elemFin) = fields(fldPos)
                  if fldFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
                    Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, " const v_")
                    writer.putSlice(outCap, outBuf, srcCap, src, bindBeg, bindFin)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
                    if paramBeg < paramFin then
                      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_")
                      writer.putSlice(outCap, outBuf, srcCap, src, paramBeg, paramFin)
                    else
                      Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "_sc")
                      Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, scrutTemp)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".u.")
                    writer.putSlice(outCap, outBuf, srcCap, src, caseNameBeg, caseNameFin)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".f_")
                    writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                    Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  (void)v_")
                    writer.putSlice(outCap, outBuf, srcCap, src, bindBeg, bindFin)
                    Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
                    if locCount < localCap then
                      val isArr: Bool = Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 5, "Array")
                      val locKind: Stigi.Emit.FieldKind = if isArr then FieldLocalArray else FieldLocal
                      locals(locCount) := FieldInfo(locKind, bindBeg, bindFin, typeBeg, typeFin, elemBeg, elemFin)
                      locCount = locCount + 1
                    else ()
                  else ()
                else ()
                fldOff = fldOff + 1
              else ()
              scanPos = scanPos + 1
        else ()
      else ()
    else ()
  else ()
  locCount
