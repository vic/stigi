// prompt: classify a Stigi type name byte slice into a closed Stigi.Emit.InferredType.
def Stigi.Emit.classifyTypeName(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, typeBeg: U64, typeFin: U64) <lessOrEqual(symCount, symCap), lessOrEqual(typeBeg, typeFin), lessOrEqual(typeFin, srcCap)>: Stigi.Emit.InferredType =
  if Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 4, "Bool") then InferBool else if Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 3, "U64") then InferWord else if Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 3, "U32") then InferWordShort else if Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 2, "U8") then InferByte else if Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 3, "I64") then InferSignedLong else if Stigi.Lex.sliceEquals(srcCap, src, typeBeg, typeFin, 3, "I32") then InferSignedWord else
    var foundEnum: Bool = false
    var enumIdx: U64 = 0
    var enumBeg: U64 = typeBeg
    var enumFin: U64 = typeFin
    var symIdx: U64 = 0
    while symIdx < symCount && !foundEnum do
      @<lessOrEqual(symIdx, symCount), lessOrEqual(enumBeg, enumFin)>
      val SymEntry(symKind, _, symBeg, symFin, _, _, _, _, _, _) = syms(symIdx)
      if symKind == SymEnum && symFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, typeBeg, typeFin, symBeg, symFin) then
        foundEnum = true
        enumIdx = symIdx
        enumBeg = symBeg
        enumFin = symFin
      else ()
      symIdx = symIdx + 1
    if foundEnum then InferEnum(enumIdx, enumBeg, enumFin) else InferNamed(typeBeg, typeFin)
