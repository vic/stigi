// prompt: emit one closed enum typedef, payload union, and structural equality helper in ISO C11.
def Stigi.Emit.emitOneEnum {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64, enumIdx: U64) <lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap), below(enumIdx, symCount)>: Unit =
  val SymEntry(_, _, enumBeg, enumFin, _, _, _, _, _, _) = syms(enumIdx)
  if enumFin <= srcCap then
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 14, "typedef enum {")
    var caseOrdinal: U64 = 0
    var hasPayload: Bool = false
    var symIdx: U64 = 0
    while symIdx < symCount do
      @<lessOrEqual(symIdx, symCount)>
      val SymEntry(symKind, ownerIdx, caseBeg, caseFin, _, _, _, subCount, _, _) = syms(symIdx)
      if symKind == SymCase && ownerIdx == enumIdx && caseFin <= srcCap then
        if subCount > 0 then hasPayload = true else ()
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
        Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
        writer.putByte(outCap, outBuf, 95)
        writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
        Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " = ")
        Stigi.Emit.writeUnsigned{region, writer}(outCap, outBuf, caseOrdinal)
        Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ",")
        if caseOrdinal < symCap then caseOrdinal = caseOrdinal + 1 else ()
      else ()
      symIdx = symIdx + 1
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "} ")
    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 5, "_Tag;")
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 16, "typedef struct {")
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "  ")
    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 9, "_Tag tag;")
    if hasPayload then
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 9, "  union {")
      var payIdx: U64 = 0
      while payIdx < symCount do
        @<lessOrEqual(payIdx, symCount)>
        val SymEntry(symKind, ownerIdx, caseBeg, caseFin, _, _, subStart, subCount, _, _) = syms(payIdx)
        if symKind == SymCase && ownerIdx == enumIdx && subCount > 0 && subStart <= fieldCount && subCount <= fieldCount && caseFin <= srcCap then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 13, "    struct { ")
          var fldOff: U64 = 0
          while fldOff < subCount do
            @<lessOrEqual(fldOff, subCount)>
            val fldPos: U64 = subStart + fldOff
            if fldPos < fieldCount then
              val FieldInfo(_, fldBeg, fldFin, typeBeg, typeFin, elemBeg, elemFin) = fields(fldPos)
              if fldFin <= srcCap && typeFin <= srcCap && elemFin <= srcCap then
                Stigi.Emit.emitType{writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, typeBeg, typeFin, elemBeg, elemFin, 0, 0)
                Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, " f_")
                writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "; ")
              else ()
            else ()
            fldOff = fldOff + 1
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "} ")
          writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        else ()
        payIdx = payIdx + 1
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 6, "  } u;")
    else ()
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 2, "} ")
    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 19, "static inline bool ")
    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "_eq(")
    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
    Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " a, ")
    Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 5, " b) {")
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 35, "  if (a.tag != b.tag) return false;")
    if hasPayload then
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 18, "  switch (a.tag) {")
      var cmpIdx: U64 = 0
      while cmpIdx < symCount do
        @<lessOrEqual(cmpIdx, symCount)>
        val SymEntry(symKind, ownerIdx, caseBeg, caseFin, _, _, subStart, subCount, _, _) = syms(cmpIdx)
        if symKind == SymCase && ownerIdx == enumIdx && subCount > 0 && subStart <= fieldCount && subCount <= fieldCount && caseFin <= srcCap then
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, "    case ")
          Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, enumBeg, enumFin)
          writer.putByte(outCap, outBuf, 95)
          writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
          Stigi.Emit.writeLit{writer}(outCap, outBuf, 9, ": return ")
          var fldOff: U64 = 0
          while fldOff < subCount do
            @<lessOrEqual(fldOff, subCount)>
            val fldPos: U64 = subStart + fldOff
            if fldOff > 0 then Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, " && ") else ()
            if fldPos < fieldCount then
              val FieldInfo(_, fldBeg, fldFin, typeBeg, typeFin, _, _) = fields(fldPos)
              if fldFin <= srcCap && typeFin <= srcCap then
                var fieldIsEnum: Bool = false
                var scanSym: U64 = 0
                while scanSym < symCount && !fieldIsEnum do
                  @<lessOrEqual(scanSym, symCount)>
                  val SymEntry(depKind, _, depBeg, depFin, _, _, _, _, _, _) = syms(scanSym)
                  if depKind == SymEnum && depFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, typeBeg, typeFin, depBeg, depFin) then
                    fieldIsEnum = true
                  else ()
                  scanSym = scanSym + 1
                if fieldIsEnum then
                  Stigi.Emit.writeMangled{writer}(outCap, outBuf, srcCap, src, typeBeg, typeFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, "_eq(a.u.")
                  writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".f_")
                  writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, ", b.u.")
                  writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".f_")
                  writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                  writer.putByte(outCap, outBuf, 41)
                else
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 5, "(a.u.")
                  writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".f_")
                  writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 8, " == b.u.")
                  writer.putSlice(outCap, outBuf, srcCap, src, caseBeg, caseFin)
                  Stigi.Emit.writeLit{writer}(outCap, outBuf, 3, ".f_")
                  writer.putSlice(outCap, outBuf, srcCap, src, fldBeg, fldFin)
                  writer.putByte(outCap, outBuf, 41)
              else Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true")
            else Stigi.Emit.writeLit{writer}(outCap, outBuf, 4, "true")
            fldOff = fldOff + 1
          Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, ";")
        else ()
        cmpIdx = cmpIdx + 1
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 25, "    default: return true;")
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 3, "  }")
    else
      Stigi.Emit.writeLine{writer}(outCap, outBuf, 14, "  return true;")
    Stigi.Emit.writeLine{writer}(outCap, outBuf, 1, "}")
  else ()
