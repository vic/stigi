// prompt: emit a C11 boolean test expression checking whether one scrutinee matches a pattern slice.
def Stigi.Emit.emitPatCond {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), patBeg: U64, patFin: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, scrutTemp: U64, paramBeg: U64, paramFin: U64) <below(0, outCap), lessOrEqual(patBeg, patFin), lessOrEqual(patFin, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(paramBeg, paramFin), lessOrEqual(paramFin, srcCap)>: Unit =
  if patBeg >= patFin then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true") else
    val Token(kind, tokBeg, tokFin, _, _) = toks(patBeg)
    if tokFin > srcCap then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true") else if kind == TokIdent && Stigi.Lex.sliceEquals(srcCap, src, tokBeg, tokFin, 1, "_") then
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true")
    else if kind == TokInt then
      writer.putByte(outCap, outBuf, 40)
      if paramBeg < paramFin then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_")
        writer.putSlice(outCap, outBuf, srcCap, src, paramBeg, paramFin)
      else
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "_sc")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, scrutTemp)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " == ")
      writer.putSlice(outCap, outBuf, srcCap, src, tokBeg, tokFin)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "ULL)")
    else if kind == TokTrue || kind == TokFalse then
      writer.putByte(outCap, outBuf, 40)
      if paramBeg < paramFin then
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_")
        writer.putSlice(outCap, outBuf, srcCap, src, paramBeg, paramFin)
      else
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "_sc")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, scrutTemp)
      if kind == TokTrue then Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, " == true)") else Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, " == false)")
    else if kind == TokUpper then
      var foundCase: Bool = false
      var enumBeg: U64 = 0
      var enumFin: U64 = 0
      var symIdx: U64 = 0
      while symIdx < symCount && !foundCase do
        @<lessOrEqual(symIdx, symCount), lessOrEqual(enumBeg, enumFin), lessOrEqual(enumFin, srcCap)>
        val SymEntry(symKind, ownerIdx, caseBeg, caseFin, _, _, _, _, _, _) = syms(symIdx)
        if symKind == SymCase && ownerIdx < symCount && caseFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, tokBeg, tokFin, caseBeg, caseFin) then
          val SymEntry(_, _, ownBeg, ownFin, _, _, _, _, _, _) = syms(ownerIdx)
          if ownFin <= srcCap then
            foundCase = true
            enumBeg = ownBeg
            enumFin = ownFin
          else ()
        else ()
        symIdx = symIdx + 1
      if foundCase then
        writer.putByte(outCap, outBuf, 40)
        if paramBeg < paramFin then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "v_")
          writer.putSlice(outCap, outBuf, srcCap, src, paramBeg, paramFin)
        else
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, "_sc")
          Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, scrutTemp)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, ".tag == ")
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
        writer.putByte(outCap, outBuf, 95)
        writer.putSlice(outCap, outBuf, srcCap, src, tokBeg, tokFin)
        writer.putByte(outCap, outBuf, 41)
      else Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true")
    else Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true")
