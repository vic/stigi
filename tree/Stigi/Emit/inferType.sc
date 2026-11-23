// prompt: infer the closed Stigi.Emit.InferredType of an expression token slice from locals and symbols.
def Stigi.Emit.inferType(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), symCount: U64, localCap: Stigi.NodeCap, locals: Array[Stigi.Emit.FieldInfo](localCap), localCount: U64, monoBeg: U64, monoFin: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(symCount, symCap), lessOrEqual(localCount, localCap), lessOrEqual(monoBeg, monoFin), lessOrEqual(monoFin, srcCap)>: Stigi.Emit.InferredType =
  var pos: U64 = startTok
  var leadDone: Bool = false
  while pos < endTok && !leadDone do
    @<lessOrEqual(pos, endTok)>
    val Token(leadKind, _, _, _, _) = toks(pos)
    if leadKind == TokLeftParen || leadKind == TokNewline then pos = pos + 1 else leadDone = true
  if pos >= endTok then InferUnknown else
    val Token(headKind, headBeg, headFin, _, _) = toks(pos)
    if headKind == TokTrue || headKind == TokFalse || headKind == TokBang then InferBool else if headKind == TokInt then InferWord else if headKind == TokChar then InferByte else if headKind == TokIdent || headKind == TokUpper then
      val TypeSpan(qualBeg, qualFin, _, _, afterQual) = Stigi.Emit.parseTypeSpan(tokCap, toks, pos, endTok)
      if qualFin > srcCap then InferUnknown else if Stigi.Lex.sliceEquals(srcCap, src, qualBeg, qualFin, 6, "U64.of") || Stigi.Lex.sliceEquals(srcCap, src, qualBeg, qualFin, 7, "U64.mix") then InferWord else if headFin <= srcCap && Stigi.Lex.sliceEquals(srcCap, src, headBeg, headFin, 3, "U64") then InferWord else if headFin <= srcCap && Stigi.Lex.sliceEquals(srcCap, src, headBeg, headFin, 3, "U32") then InferWordShort else if headFin <= srcCap && Stigi.Lex.sliceEquals(srcCap, src, headBeg, headFin, 2, "U8") then InferByte else if headFin <= srcCap && Stigi.Lex.sliceEquals(srcCap, src, headBeg, headFin, 3, "I64") then InferSignedLong else if headFin <= srcCap && Stigi.Lex.sliceEquals(srcCap, src, headBeg, headFin, 3, "I32") then InferSignedWord else
        val isSingleIdent: Bool = qualBeg == headBeg && qualFin == headFin
        val hasParenNext: Bool = if afterQual < endTok then
          val Token(afterKind, _, _, _, _) = toks(afterQual)
          afterKind == TokLeftParen
        else false
        var localIdx: U64 = localCount
        var foundLocal: Bool = false
        var locTypeBeg: U64 = 0
        var locTypeFin: U64 = 0
        var locCapBeg: U64 = 0
        var locCapFin: U64 = 0
        var isCapCall: Bool = false
        while localIdx > 0 && !foundLocal do
          @<lessOrEqual(localIdx, localCount), lessOrEqual(locTypeBeg, locTypeFin), lessOrEqual(locTypeFin, srcCap), lessOrEqual(locCapBeg, locCapFin), lessOrEqual(locCapFin, srcCap)>
          localIdx = localIdx - 1
          val FieldInfo(locKind, locNameBeg, locNameFin, typeBeg, typeFin, elemBeg, elemFin) = locals(localIdx)
          if locNameFin <= srcCap && headFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, headBeg, headFin, locNameBeg, locNameFin) then
            if isSingleIdent then
              foundLocal = true
              if locKind == FieldLocalArray && hasParenNext && elemFin <= srcCap then
                locTypeBeg = elemBeg
                locTypeFin = elemFin
              else if typeFin <= srcCap then
                locTypeBeg = typeBeg
                locTypeFin = typeFin
              else ()
            else if locKind == FieldLocalCap && typeFin <= srcCap then
              foundLocal = true
              isCapCall = true
              locCapBeg = typeBeg
              locCapFin = typeFin
            else ()
          else ()
        if foundLocal && !isCapCall then
          val useBeg: U64 = if monoBeg < monoFin && locTypeFin - locTypeBeg == 1 then monoBeg else locTypeBeg
          val useFin: U64 = if monoBeg < monoFin && locTypeFin - locTypeBeg == 1 then monoFin else locTypeFin
          if useBeg <= useFin then Stigi.Emit.classifyTypeName(srcCap, src, symCap, syms, symCount, useBeg, useFin) else InferUnknown
        else if foundLocal && isCapCall then
          val mtdPos: U64 = pos + 2
          if mtdPos < endTok then
            val Token(_, mtdBeg, mtdFin, _, _) = toks(mtdPos)
            var symIdx: U64 = 0
            var retBeg: U64 = 0
            var retFin: U64 = 0
            var foundMtd: Bool = false
            while symIdx < symCount && !foundMtd do
              @<lessOrEqual(symIdx, symCount), lessOrEqual(retBeg, retFin), lessOrEqual(retFin, srcCap)>
              val SymEntry(symKind, ownerIdx, symBeg, symFin, mtdRetBeg, mtdRetFin, _, _, _, _) = syms(symIdx)
              if symKind == SymCapMethod && ownerIdx < symCount && symFin <= srcCap && mtdFin <= srcCap && mtdRetFin <= srcCap then
                val SymEntry(_, _, capBeg, capFin, _, _, _, _, _, _) = syms(ownerIdx)
                if capFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, locCapBeg, locCapFin, capBeg, capFin) && Stigi.Emit.sliceEqual(srcCap, src, mtdBeg, mtdFin, symBeg, symFin) then
                  foundMtd = true
                  retBeg = mtdRetBeg
                  retFin = mtdRetFin
                else ()
              else ()
              symIdx = symIdx + 1
            if foundMtd then Stigi.Emit.classifyTypeName(srcCap, src, symCap, syms, symCount, retBeg, retFin) else InferUnknown
          else InferUnknown
        else
          var symIdx: U64 = 0
          var foundSym: Bool = false
          var foundEnumIdx: U64 = 0
          var foundIsEnum: Bool = false
          var resBeg: U64 = 0
          var resFin: U64 = 0
          while symIdx < symCount && !foundSym do
            @<lessOrEqual(symIdx, symCount), lessOrEqual(resBeg, resFin), lessOrEqual(resFin, srcCap)>
            val SymEntry(symKind, ownerIdx, symBeg, symFin, retBeg, retFin, _, _, _, _) = syms(symIdx)
            val isValueSym: Bool = symKind == SymFunc || symKind == SymExternBody || symKind == SymVal
            if symKind == SymCase && ownerIdx < symCount && symFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, qualBeg, qualFin, symBeg, symFin) then
              val SymEntry(_, _, enumBeg, enumFin, _, _, _, _, _, _) = syms(ownerIdx)
              if enumFin <= srcCap then
                foundSym = true
                foundIsEnum = true
                foundEnumIdx = ownerIdx
                resBeg = enumBeg
                resFin = enumFin
              else ()
            else if isValueSym && symFin <= srcCap && retFin <= srcCap && Stigi.Emit.sliceEqual(srcCap, src, qualBeg, qualFin, symBeg, symFin) then
              foundSym = true
              resBeg = retBeg
              resFin = retFin
            else ()
            symIdx = symIdx + 1
          if foundIsEnum then InferEnum(foundEnumIdx, resBeg, resFin) else if foundSym then Stigi.Emit.classifyTypeName(srcCap, src, symCap, syms, symCount, resBeg, resFin) else InferUnknown
    else InferUnknown
