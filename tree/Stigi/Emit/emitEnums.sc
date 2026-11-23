// prompt: emit all indexed enums in topological dependency order so embedded enums precede parents.
def Stigi.Emit.emitEnums {region: Stigi.Region, writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), fieldCount: U64) <lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>: Unit =
  val Filled(doneFlag) = Array.tryFill[Bool]{region}(symCap, false) else
    case FillFailed => ()
  var passNum: U64 = 0
  while passNum < 32 do
    @<lessOrEqual(passNum, 32)>
    var enumIdx: U64 = 0
    while enumIdx < symCount do
      @<lessOrEqual(enumIdx, symCount)>
      val SymEntry(symKind, _, _, _, _, _, _, _, _, _) = syms(enumIdx)
      if symKind == SymEnum && !doneFlag(enumIdx) then
        var ready: Bool = true
        var caseIdx: U64 = 0
        while caseIdx < symCount && ready do
          @<lessOrEqual(caseIdx, symCount)>
          val SymEntry(caseKind, ownerIdx, _, _, _, _, subStart, subCount, _, _) = syms(caseIdx)
          if caseKind == SymCase && ownerIdx == enumIdx && subStart <= fieldCount && subCount <= fieldCount then
            var fldOff: U64 = 0
            while fldOff < subCount && ready do
              @<lessOrEqual(fldOff, subCount)>
              val fldPos: U64 = subStart + fldOff
              if fldPos < fieldCount then
                val FieldInfo(_, _, _, typeBeg, typeFin, _, _) = fields(fldPos)
                if typeFin <= srcCap then
                  var depIdx: U64 = 0
                  while depIdx < symCount && ready do
                    @<lessOrEqual(depIdx, symCount)>
                    val SymEntry(depKind, _, depBeg, depFin, _, _, _, _, _, _) = syms(depIdx)
                    if depKind == SymEnum && depFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, typeBeg, typeFin, depBeg, depFin) then
                      if !doneFlag(depIdx) then ready = false else ()
                    else ()
                    depIdx = depIdx + 1
                else ()
              else ()
              fldOff = fldOff + 1
          else ()
          caseIdx = caseIdx + 1
        if ready then
          Stigi.Emit.emitOneEnum{region, writer}(outCap, outBuf, srcCap, src, symCap, syms, symCount, fieldCap, fields, fieldCount, enumIdx)
          doneFlag(enumIdx) := true
        else ()
      else ()
      enumIdx = enumIdx + 1
    passNum = passNum + 1
