// prompt: emit global val constants and forward prototypes for defs, externs, givens, and tests.
def Stigi.Emit.emitPrototypes {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64) <lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>: Unit =
  var symIdx: U64 = 0
  while symIdx < symCount do
    @<lessOrEqual(symIdx, symCount)>
    val SymEntry(symKind, ownerIdx, nameBeg, nameFin, retBeg, retFin, subStart, subCount, bodyStart, bodyEnd) = syms(symIdx)
    if nameFin <= srcCap && retFin <= srcCap && subStart <= fieldCount && subCount <= fieldCount then
      if symKind == SymVal && bodyStart < bodyEnd && bodyStart < tokCap then
        val Token(_, litBeg, litFin, _, _) = toks(bodyStart)
        if litFin <= srcCap then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "#define ")
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, " (")
          writer.putSlice(outCap, outBuf, srcCap, src, litBeg, litFin)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 4, "ULL)")
        else ()
      else if symKind == SymTest then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 15, "static int32_t ")
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 7, "(void);")
      else if symKind == SymFunc || symKind == SymExternBody || symKind == SymGivenMethod then
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
        else
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, nameBeg, nameFin)
        writer.putByte(outCap, outBuf, 40)
        if subCount == 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "void") else
          var parOff: U64 = 0
          while parOff < subCount do
            @<lessOrEqual(parOff, subCount)>
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
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
      else if symKind == SymGenericFunc then
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
                @<lessOrEqual(parOff, subCount)>
                val parPos: U64 = subStart + parOff
                if parOff > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, ", ") else ()
                if parPos < fieldCount then
                  val FieldInfo(_, parBeg, parFin, typeBeg, typeFin, elemBeg, elemFin) = fields(parPos)
                  if parFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
                    Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, tgtBeg, tgtFin)
                    Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " v_")
                    writer.putSlice(outCap, outBuf, srcCap, src, parBeg, parFin)
                  else ()
                else ()
                parOff = parOff + 1
              Stigi.Emit.writeLine{writer}(outCap, outBuf, 2, ");")
            else ()
          else ()
          givIdx = givIdx + 1
      else ()
    else ()
    symIdx = symIdx + 1
