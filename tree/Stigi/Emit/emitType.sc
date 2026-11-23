// prompt: lower any Stigi type byte span into its exact C11 type through Stigi.Emit.Writer.
def Stigi.Emit.emitType {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, typeStart: U64, typeEnd: U64, elemStart: U64, elemEnd: U64, monoStart: U64, monoEnd: U64) <lessOrEqual(symCount, symCap), lessOrEqual(typeStart, typeEnd), lessOrEqual(typeEnd, srcCap), lessOrEqual(elemStart, elemEnd), lessOrEqual(elemEnd, srcCap), lessOrEqual(monoStart, monoEnd), lessOrEqual(monoEnd, srcCap)>: Unit =
  val isArray: Bool = Stigi.Lex.sliceEquals(srcCap, src, typeStart, typeEnd, 5, "Array")
  if isArray then
    if Stigi.Lex.sliceEquals(srcCap, src, elemStart, elemEnd, 3, "U64") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "uint64_t *") else if Stigi.Lex.sliceEquals(srcCap, src, elemStart, elemEnd, 3, "U32") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 10, "uint32_t *") else if Stigi.Lex.sliceEquals(srcCap, src, elemStart, elemEnd, 2, "U8") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "uint8_t *") else if Stigi.Lex.sliceEquals(srcCap, src, elemStart, elemEnd, 3, "I64") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "int64_t *") else if Stigi.Lex.sliceEquals(srcCap, src, elemStart, elemEnd, 3, "I32") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "int32_t *") else if Stigi.Lex.sliceEquals(srcCap, src, elemStart, elemEnd, 4, "Bool") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "bool *") else
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, elemStart, elemEnd)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, " *")
  else
    val useMono: Bool = monoStart < monoEnd && typeEnd - typeStart == 1
    var resolvedStart: U64 = typeStart
    var resolvedEnd: U64 = typeEnd
    if useMono then
      resolvedStart = monoStart
      resolvedEnd = monoEnd
    else ()
    var isCap: Bool = false
    var symIdx: U64 = 0
    while symIdx < symCount do
      @<lessOrEqual(symIdx, symCount), lessOrEqual(resolvedStart, resolvedEnd), lessOrEqual(resolvedEnd, srcCap)>
      val SymEntry(symKind, _, symBeg, symFin, auxBeg, auxFin, _, _, _, _) = syms(symIdx)
      if symFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, resolvedStart, resolvedEnd, symBeg, symFin) then
        if symKind == SymTypeAlias && auxFin <= srcCap then
          resolvedStart = auxBeg
          resolvedEnd = auxFin
        else if symKind == SymCap then
          isCap = true
        else ()
      else ()
      symIdx = symIdx + 1
    if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 3, "U64") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint64_t") else if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 3, "U32") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "uint32_t") else if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 2, "U8") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "uint8_t") else if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 3, "I64") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "int64_t") else if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 3, "I32") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "int32_t") else if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 4, "Bool") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "bool") else if Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 4, "Unit") || Stigi.Lex.sliceEquals(srcCap, src, resolvedStart, resolvedEnd, 5, "Never") then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "void") else if isCap then
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, resolvedStart, resolvedEnd)
      Stigi.Emit.writeLit{writer}(outCap, outBuf, 7, "_Handle")
    else
      Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, resolvedStart, resolvedEnd)
