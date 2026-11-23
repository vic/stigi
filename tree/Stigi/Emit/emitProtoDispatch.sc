// prompt: emit handler method forward declarations and static inline capability protocol dispatchers.
def Stigi.Emit.emitProtoDispatch {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64) <lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>: Unit =
  var mtdIdx: U64 = 0
  while mtdIdx < symCount do
    @<lessOrEqual(mtdIdx, symCount)>
    val SymEntry(symKind, ownerIdx, mtdBeg, mtdFin, retBeg, retFin, subStart, subCount, _, _) = syms(mtdIdx)
    if symKind == SymHandlerMethod && ownerIdx < symCount && mtdFin <= srcCap && retFin <= srcCap && subStart <= fieldCount && subCount <= fieldCount then
      val SymEntry(_, _, hndBeg, hndFin, _, _, _, _, _, _) = syms(ownerIdx)
      if hndFin <= srcCap then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, "static inline ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, 0, 0)
        writer.putByte(outCap, outBuf, 32)
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "_op_")
        writer.putSlice(outCap, outBuf, srcCap, src, mtdBeg, mtdFin)
        writer.putByte(outCap, outBuf, 40)
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 15, "_State *_hstate")
        var parOff: U64 = 0
        while parOff < subCount do
          @<lessOrEqual(parOff, subCount)>
          val parPos: U64 = subStart + parOff
          if parPos < fieldCount then
            val FieldInfo(_, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin) = fields(parPos)
            if parFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
              Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
              writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
            else ()
          else ()
          parOff = parOff + 1
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
      else ()
    else ()
    mtdIdx = mtdIdx + 1
  var capMtd: U64 = 0
  while capMtd < symCount do
    @<lessOrEqual(capMtd, symCount)>
    val SymEntry(symKind, capIdx, mtdBeg, mtdFin, retBeg, retFin, subStart, subCount, _, _) = syms(capMtd)
    if symKind == SymCapMethod && capIdx < symCount && mtdFin <= srcCap && retFin <= srcCap && subStart <= fieldCount && subCount <= fieldCount then
      val SymEntry(_, _, capBeg, capFin, _, _, _, _, _, _) = syms(capIdx)
      if capFin <= srcCap then
        val isVoidRet: Bool = Stigi.Lex.sliceEquals(srcCap, src, retBeg, retFin, 4, "Unit")
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, "static inline ")
        Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, 0, 0)
        writer.putByte(outCap, outBuf, 32)
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, capBeg, capFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "_proto_")
        writer.putSlice(outCap, outBuf, srcCap, src, mtdBeg, mtdFin)
        writer.putByte(outCap, outBuf, 40)
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, capBeg, capFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "_Handle _h")
        var parOff: U64 = 0
        while parOff < subCount do
          @<lessOrEqual(parOff, subCount)>
          val parPos: U64 = subStart + parOff
          if parPos < fieldCount then
            val FieldInfo(_, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin) = fields(parPos)
            if parFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ")
              Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
              Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
              writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
            else ()
          else ()
          parOff = parOff + 1
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
        var tagNum: U64 = 0
        var scanHnd: U64 = 0
        while scanHnd < symCount do
          @<lessOrEqual(scanHnd, symCount)>
          val SymEntry(hndKind, _, hndBeg, hndFin, targetBeg, targetFin, _, _, _, _) = syms(scanHnd)
          if hndKind == SymHandler && hndFin <= srcCap && targetFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, capBeg, capFin, targetBeg, targetFin) then
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 16, "  if (_h.tag == ")
            Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tagNum)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, "U) { ")
            if !isVoidRet then Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "return ") else ()
            Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, hndBeg, hndFin)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "_op_")
            writer.putSlice(outCap, outBuf, srcCap, src, mtdBeg, mtdFin)
            Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "(_h.ptr.h_")
            Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, tagNum)
            var argOff: U64 = 0
            while argOff < subCount do
              @<lessOrEqual(argOff, subCount)>
              val parPos: U64 = subStart + argOff
              if parPos < fieldCount then
                val FieldInfo(_, parBeg, parFin, _, _, _, _) = fields(parPos)
                if parFin <= srcCap then
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, ", v_")
                  writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
                else ()
              else ()
              argOff = argOff + 1
            if isVoidRet then Stigi.Emit.writeLine{writer}(outCap, outBuf, 12, "); return; }") else Stigi.Emit.writeLine{writer}(outCap, outBuf, 4, "); }")
            if tagNum < symCap then tagNum = tagNum + 1 else ()
          else ()
          scanHnd = scanHnd + 1
        if !isVoidRet then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
          Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, 0, 0)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 21, " _z = {0}; return _z;")
        else ()
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
      else ()
    else ()
    capMtd = capMtd + 1
