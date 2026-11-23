// prompt: emit capability handle structs, handler state structs, init helpers, and proto dispatchers.
def Stigi.Emit.emitCapsAndHandlers {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64) <lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>: Unit =
  var hndIdx: U64 = 0
  while hndIdx < symCount do
    @<lessOrEqual(hndIdx, symCount)>
    val SymEntry(symKind, paramCount, hndBeg, hndFin, _, _, subStart, subCount, _, _) = syms(hndIdx)
    if symKind == SymHandler && hndFin <= srcCap && subStart <= fieldCount && subCount <= fieldCount && paramCount <= fieldCount then
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 16, "typedef struct {")
      var varCount: U64 = 0
      var fldOff: U64 = 0
      while fldOff < subCount do
        @<lessOrEqual(fldOff, subCount)>
        val fldPos: U64 = subStart + fldOff
        if fldPos < fieldCount then
          val FieldInfo(fldKind, fldBeg, fldFin, typeBeg, typeFin, elemBeg, elemFin) = fields(fldPos)
          val isStateOrParam: Bool = fldKind == FieldParam && fldOff < paramCount || fldKind == FieldStateVar
          if isStateOrParam && fldFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
            Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " f_")
            writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
            if varCount < fieldCap then varCount = varCount + 1 else ()
          else ()
        else ()
        fldOff = fldOff + 1
      if varCount == 0 then Stigi.Emit.writeLine{writer}(outCap, outBuf, 16, "  uint8_t dummy;") else ()
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "} ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 7, "_State;")
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, "static inline ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "_State ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "_init(")
      if paramCount == 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "void") else
        var parOff: U64 = 0
        while parOff < paramCount do
          @<lessOrEqual(parOff, paramCount)>
          val parPos: U64 = subStart + parOff
          if parOff > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
          if parPos < fieldCount then
            val FieldInfo(_, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin) = fields(parPos)
            if parFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
              Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
              writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
            else ()
          else ()
          parOff = parOff + 1
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 17, "_State _st = {0};")
      var initOff: U64 = 0
      while initOff < subCount do
        @<lessOrEqual(initOff, subCount)>
        val fldPos: U64 = subStart + initOff
        if fldPos < fieldCount then
          val FieldInfo(fldKind, fldBeg, fldFin, _, _, initBeg, initFin) = fields(fldPos)
          if fldKind == FieldParam && initOff < paramCount && fldFin <= srcCap then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "  _st.f_")
            writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, " = v_")
            writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
          else if fldKind == FieldStateVar && fldFin <= srcCap && initBeg < initFin && initFin <= srcCap then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "  _st.f_")
            writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
            val firstByte: U8 = src(initBeg)
            val isLitOrBool: Bool = firstByte >= 48 && firstByte <= 57 || Stigi.Lex.sliceEquals(srcCap, src, initBeg, initFin, 4, "true") || Stigi.Lex.sliceEquals(srcCap, src, initBeg, initFin, 5, "false")
            if !isLitOrBool then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_") else ()
            writer.putSlice(outCap, outBuf, srcCap, src, initBeg, initFin)
            Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
          else ()
        else ()
        initOff = initOff + 1
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 13, "  return _st;")
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
    else ()
    hndIdx = hndIdx + 1
  var capIdx: U64 = 0
  while capIdx < symCount do
    @<lessOrEqual(capIdx, symCount)>
    val SymEntry(symKind, _, capBeg, capFin, _, _, _, _, _, _) = syms(capIdx)
    if symKind == SymCap && capFin <= srcCap then
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 38, "typedef struct { uint32_t tag; union {")
      var tagNum: U64 = 0
      var scanHnd: U64 = 0
      while scanHnd < symCount do
        @<lessOrEqual(scanHnd, symCount)>
        val SymEntry(hndKind, _, hndStart, hndEnd, targetBeg, targetFin, _, _, _, _) = syms(scanHnd)
        if hndKind == SymHandler && hndEnd <= srcCap && targetFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, capBeg, capFin, targetBeg, targetFin) then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndStart, hndEnd)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "_State *h_")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tagNum)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
          if tagNum < symCap then tagNum = tagNum + 1 else ()
        else ()
        scanHnd = scanHnd + 1
      if tagNum == 0 then Stigi.Emit.writeLine{writer}(outCap, outBuf, 16, "  uint8_t dummy;") else ()
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "} ptr; } ")
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, capBeg, capFin)
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 8, "_Handle;")
    else ()
    capIdx = capIdx + 1
