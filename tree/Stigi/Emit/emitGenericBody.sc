// prompt: emit monomorphized C11 definitions for a generic trait-constrained def across all concrete givens.
def Stigi.Emit.emitGenericBody {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, locals: Array[Stigi.Emit.FieldInfo](256), symIdx: U64) <below(0, outCap), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), below(symIdx, symCount)>: Unit =
  val SymEntry(_, _, nameBeg, nameFin, retBeg, retFin, subStart, subCount, bodyStart, bodyEnd) = syms(symIdx)
  if nameFin <= srcCap && retFin <= srcCap && subStart <= fieldCount && subCount <= fieldCount && bodyEnd <= tokCap then
    var givIdx: U64 = 0
    while givIdx < symCount do
      @<lessOrEqual(givIdx, symCount)>
      val SymEntry(givKind, _, _, _, tgtBeg, tgtFin, _, _, _, _) = syms(givIdx)
      if givKind == SymGiven && tgtBeg < tgtFin && tgtFin <= srcCap then
        var seenEarlier: Bool = false
        var prevIdx: U64 = 0
        while prevIdx < givIdx && !seenEarlier do
          @<lessOrEqual(prevIdx, givIdx)>
          val SymEntry(prevKind, _, _, _, prevBeg, prevFin, _, _, _, _) = syms(prevIdx)
          if prevKind == SymGiven && prevFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, tgtBeg, tgtFin, prevBeg, prevFin) then
            seenEarlier = true
          else ()
          prevIdx = prevIdx + 1
        if !seenEarlier then
          var locCount: U64 = 0
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 14, "static inline ")
          Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, retBeg, retFin, 0, 0, tgtBeg, tgtFin)
          writer.putByte(outCap, outBuf, 32)
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "__")
          var dotPos: U64 = tgtBeg
          while dotPos < tgtFin do
            @<lessOrEqual(tgtBeg, dotPos), lessOrEqual(dotPos, tgtFin)>
            val curByte: U8 = src(dotPos)
            writer.putByte(outCap, outBuf, if curByte == 46 then 95 else curByte)
            dotPos = dotPos + 1
          writer.putByte(outCap, outBuf, 40)
          var parOff: U64 = 0
          while parOff < subCount do
            @<lessOrEqual(parOff, subCount), lessOrEqual(locCount, 256)>
            val parPos: U64 = subStart + parOff
            if parOff > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
            if parPos < fieldCount then
              val FieldInfo(_, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin) = fields(parPos)
              if parFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
                Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, tgtBeg, tgtFin)
                Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
                writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
                if locCount < 256 then
                  locals(locCount) := FieldInfo(FieldParam, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin)
                  locCount = locCount + 1
                else ()
              else ()
            else ()
            parOff = parOff + 1
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, ") {")
          val meta: Stigi.Emit.BodyMeta = BodyMeta(retBeg, retFin, tgtBeg, tgtFin, false, false, 0)
          Stigi.Emit.emitStmt{region, writer}(outCap, outBuf, srcCap, src, tokCap, toks, bodyStart, bodyEnd, symCap, syms, symCount, fieldCap, fields, fieldCount, 256, locals, locCount, meta, TargetReturn, 64)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
        else ()
      else ()
      givIdx = givIdx + 1
  else ()
