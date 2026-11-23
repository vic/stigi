// prompt: emit one C11 function body for a normal def, extern body, given method, handler method, or test.
def Stigi.Emit.emitOneBody {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, locals: Array[Stigi.Emit.FieldInfo](256), symIdx: U64) <below(0, outCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), below(symIdx, symCount)>: Unit =
  val SymEntry(symKind, ownerIdx, nameBeg, nameFin, retBeg, retFin, subStart, subCount, bodyStart, bodyEnd) = syms(symIdx)
  if nameFin <= srcCap && retFin <= srcCap && subStart <= fieldCount && subCount <= fieldCount && bodyEnd <= tokCap then
    var locCount: U64 = 0
    if symKind == SymTest then
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 15, "static int32_t ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 8, "(void) {")
    else
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, "static inline ")
      Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, 0, 0)
      writer.putByte(outCap, outBuf, 32)
      if symKind == SymGivenMethod && ownerIdx < symCount then
        val SymEntry(_, _, givBeg, givFin, _, _, _, _, _, _) = syms(ownerIdx)
        if givFin <= srcCap then
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, givBeg, givFin)
          writer.putByte(outCap, outBuf, 95)
          writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        else ()
      else if symKind == SymHandlerMethod && ownerIdx < symCount then
        val SymEntry(_, hndParamCount, hndBeg, hndFin, _, _, hndSubStart, hndSubCount, _, _) = syms(ownerIdx)
        if hndFin <= srcCap then
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "_op_")
          writer.putSlice(outCap, outBuf, srcCap, src, nameBeg, nameFin)
          if hndSubStart <= fieldCount && hndSubCount <= fieldCount then
            var stateOff: U64 = 0
            while stateOff < hndSubCount && locCount < 256 do
              @<lessOrEqual(stateOff, hndSubCount), lessOrEqual(locCount, 256)>
              val statePos: U64 = hndSubStart + stateOff
              if statePos < fieldCount then
                val FieldInfo(stateFldKind, fldBeg, fldFin, typeBeg, typeFin, elemBeg, elemFin) = fields(statePos)
                val isHndField: Bool = stateFldKind == FieldParam && stateOff < hndParamCount || stateFldKind == FieldStateVar
                if isHndField then
                  locals(locCount) := FieldInfo(FieldStateVar, fldBeg, fldFin, typeBeg, typeFin, elemBeg, elemFin)
                  locCount = locCount + 1
                else ()
              else ()
              stateOff = stateOff + 1
          else ()
        else ()
      else
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
      writer.putByte(outCap, outBuf, 40)
      var emittedParams: U64 = 0
      if symKind == SymHandlerMethod && ownerIdx < symCount then
        val SymEntry(_, _, hndBeg, hndFin, _, _, _, _, _, _) = syms(ownerIdx)
        if hndFin <= srcCap then
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 15, "_State *_hstate")
          emittedParams = 1
        else ()
      else ()
      if subCount == 0 && emittedParams == 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "void") else
        var parOff: U64 = 0
        while parOff < subCount do
          @<lessOrEqual(parOff, subCount), lessOrEqual(locCount, 256)>
          val parPos: U64 = subStart + parOff
          if emittedParams > 0 || parOff > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
          if parPos < fieldCount then
            val FieldInfo(parKind, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin) = fields(parPos)
            if parFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
              Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
              writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
              if locCount < 256 then
                val isArr: Bool = Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 5, "Array")
                val bindKind: Stigi.Emit.FieldKind = if parKind == FieldCapParam then FieldLocalCap else if isArr then FieldLocalArray else FieldParam
                locals(locCount) := FieldInfo(bindKind, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin)
                locCount = locCount + 1
              else ()
            else ()
          else ()
          parOff = parOff + 1
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
      if symKind == SymHandlerMethod then Stigi.Emit.writeLine{writer}(outCap, outBuf, 16, "  (void)_hstate;") else ()
      var voidOff: U64 = 0
      while voidOff < subCount do
        @<lessOrEqual(voidOff, subCount)>
        val parPos: U64 = subStart + voidOff
        if parPos < fieldCount then
          val FieldInfo(_, parBeg, parFin, _, _, _, _) = fields(parPos)
          if parFin <= srcCap then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "  (void)v_")
            writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
          else ()
        else ()
        voidOff = voidOff + 1
    val meta: Stigi.Emit.BodyMeta = BodyMeta(retBeg, retFin, 0, 0, symKind == SymTest, symKind == SymHandlerMethod, ownerIdx)
    Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, bodyStart, bodyEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, 256, locals, locCount, meta, TargetReturn, 64)
    val isVoidRet: Bool = symKind != SymTest && Stigi.Lex.sliceEquals(srcCap, src, retBeg, retFin, 4, "Unit")
    if symKind == SymTest then Stigi.Emit.writeLine{writer}(outCap, outBuf, 11, "  return 0;") else if !isVoidRet then
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
      Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, 0, 0)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 27, " _retz = {0}; return _retz;")
    else ()
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
  else ()
