// prompt: index one loaded L0 unit token range into the unified emitter symbol and field tables.
def Stigi.Emit.indexUnit {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), initSyms: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), initFields: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(initSyms, symCap), lessOrEqual(initFields, fieldCap)>: Stigi.Emit.IndexCounts =
  var pos: U64 = startTok
  var symCount: U64 = initSyms
  var fieldCount: U64 = initFields
  var leadDone: Bool = false
  while pos < endTok && !leadDone do
    @<lessOrEqual(pos, endTok)>
    val Token(leadKind, _, _, _, _) = toks(pos)
    if leadKind == TokNewline || leadKind == TokPub then pos = pos + 1 else leadDone = true
  if pos < endTok then
    val Token(declTok, _, _, _, _) = toks(pos)
    if declTok == TokDef then
      val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.parseDefSig(srcCap, src, tokCap, toks, pos + 1, endTok, SymFunc, 0, symCap, syms, symCount, fieldCap, fields, fieldCount)
      if nextSyms <= symCap then symCount = nextSyms else ()
      if nextFields <= fieldCap then fieldCount = nextFields else ()
    else if declTok == TokExtern then
      var defPos: U64 = pos + 1
      var foundDef: Bool = false
      while defPos < endTok && !foundDef do
        @<lessOrEqual(defPos, endTok), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>
        val Token(extKind, _, _, _, _) = toks(defPos)
        if extKind == TokDef then
          val nextDefPos: U64 = defPos + 1
          val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.parseDefSig(srcCap, src, tokCap, toks, nextDefPos, endTok, SymExternBody, 0, symCap, syms, symCount, fieldCap, fields, fieldCount)
          if nextSyms <= symCap then symCount = nextSyms else ()
          if nextFields <= fieldCap then fieldCount = nextFields else ()
          foundDef = true
        else
          defPos = defPos + 1
    else if declTok == TokEnum || declTok == TokCapability || declTok == TokHandler || declTok == TokGiven || declTok == TokType || declTok == TokVal then
      val TypeSpan(nameBeg, nameFin, _, _, afterName) = Stigi.Emit.parseTypeSpan(tokCap, toks, pos + 1, endTok)
      var cursor: U64 = if afterName <= endTok then afterName else endTok
      if declTok == TokType then
        var foundAssign: Bool = false
        while cursor < endTok && !foundAssign do
          @<lessOrEqual(cursor, endTok)>
          val Token(scanKind, _, _, _, _) = toks(cursor)
          cursor = cursor + 1
          if scanKind == TokAssign then foundAssign = true else ()
        if foundAssign && cursor < endTok then
          val TypeSpan(baseBeg, baseFin, _, _, _) = Stigi.Emit.parseTypeSpan(tokCap, toks, cursor, endTok)
          if symCount < symCap then
            syms(symCount) := SymEntry(SymTypeAlias, 0, nameBeg, nameFin, baseBeg, baseFin, 0, 0, 0, 0)
            symCount = symCount + 1
          else ()
        else ()
      else if declTok == TokVal then
        var isTest: Bool = false
        var isNegTest: Bool = false
        var typeBeg: U64 = 0
        var typeFin: U64 = 0
        var bodyStart: U64 = endTok
        var valDone: Bool = false
        while cursor < endTok && !valDone do
          @<lessOrEqual(cursor, endTok), lessOrEqual(typeBeg, typeFin), lessOrEqual(bodyStart, endTok)>
          val Token(scanKind, tokBeg, tokFin, _, _) = toks(cursor)
          if scanKind == TokUpper && tokFin <= srcCap then
            if Stigi.Lex.sliceEquals(srcCap, src, tokBeg, tokFin, 4, "Test") then isTest = true else if Stigi.Lex.sliceEquals(srcCap, src, tokBeg, tokFin, 7, "NegTest") then isNegTest = true else ()
            cursor = cursor + 1
          else if scanKind == TokColon && cursor + 1 < endTok then
            val TypeSpan(valTypeBeg, valTypeFin, _, _, afterType) = Stigi.Emit.parseTypeSpan(tokCap, toks, cursor + 1, endTok)
            typeBeg = valTypeBeg
            typeFin = valTypeFin
            cursor = if afterType > cursor && afterType <= endTok then afterType else cursor + 1
          else if scanKind == TokAssign then
            val nextCur: U64 = cursor + 1
            bodyStart = nextCur
            valDone = true
          else
            cursor = cursor + 1
        if !isNegTest && symCount < symCap then
          val valKind: Stigi.Emit.SymKind = if isTest then SymTest else SymVal
          syms(symCount) := SymEntry(valKind, 0, nameBeg, nameFin, typeBeg, typeFin, 0, 0, bodyStart, endTok)
          symCount = symCount + 1
        else ()
      else if declTok == TokEnum then
        val ownerIdx: U64 = symCount
        if symCount < symCap then
          syms(symCount) := SymEntry(SymEnum, 0, nameBeg, nameFin, 0, 0, 0, 0, 0, 0)
          symCount = symCount + 1
          val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.indexMembers(srcCap, src, tokCap, toks, cursor, endTok, SymEnum, ownerIdx, symCap, syms, symCount, fieldCap, fields, fieldCount)
          if nextSyms <= symCap then symCount = nextSyms else ()
          if nextFields <= fieldCap then fieldCount = nextFields else ()
        else ()
      else if declTok == TokCapability then
        val ownerIdx: U64 = symCount
        if symCount < symCap then
          syms(symCount) := SymEntry(SymCap, 0, nameBeg, nameFin, 0, 0, 0, 0, 0, 0)
          symCount = symCount + 1
          val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.indexMembers(srcCap, src, tokCap, toks, cursor, endTok, SymCap, ownerIdx, symCap, syms, symCount, fieldCap, fields, fieldCount)
          if nextSyms <= symCap then symCount = nextSyms else ()
          if nextFields <= fieldCap then fieldCount = nextFields else ()
        else ()
      else if declTok == TokHandler then
        val ownerIdx: U64 = symCount
        val subStart: U64 = fieldCount
        val IndexCounts(_, afterParams, afterPos) = Stigi.Emit.parseParamList(tokCap, toks, cursor, endTok, FieldParam, fieldCap, fields, fieldCount)
        if afterParams <= fieldCap then fieldCount = afterParams else ()
        if afterPos <= endTok then cursor = afterPos else ()
        var capBeg: U64 = 0
        var capFin: U64 = 0
        var foundFor: Bool = false
        while cursor < endTok && !foundFor do
          @<lessOrEqual(cursor, endTok), lessOrEqual(capBeg, capFin)>
          val Token(forKind, _, _, _, _) = toks(cursor)
          cursor = cursor + 1
          if forKind == TokFor && cursor < endTok then
            val TypeSpan(targetBeg, targetFin, _, _, afterCap) = Stigi.Emit.parseTypeSpan(tokCap, toks, cursor, endTok)
            capBeg = targetBeg
            capFin = targetFin
            cursor = if afterCap <= endTok then afterCap else endTok
            foundFor = true
          else ()
        if symCount < symCap then
          val paramEndField: U64 = fieldCount
          syms(symCount) := SymEntry(SymHandler, 0, nameBeg, nameFin, capBeg, capFin, subStart, 0, 0, 0)
          symCount = symCount + 1
          val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.indexMembers(srcCap, src, tokCap, toks, cursor, endTok, SymHandler, ownerIdx, symCap, syms, symCount, fieldCap, fields, fieldCount)
          if nextSyms <= symCap then symCount = nextSyms else ()
          if nextFields <= fieldCap then fieldCount = nextFields else ()
          val totalFields: U64 = if subStart <= fieldCount then fieldCount - subStart else 0
          val paramCount: U64 = if subStart <= paramEndField then paramEndField - subStart else 0
          syms(ownerIdx) := SymEntry(SymHandler, paramCount, nameBeg, nameFin, capBeg, capFin, subStart, totalFields, 0, 0)
        else ()
      else
        val ownerIdx: U64 = symCount
        var targetBeg: U64 = 0
        var targetFin: U64 = 0
        var foundColon: Bool = false
        while cursor < endTok && !foundColon do
          @<lessOrEqual(cursor, endTok), lessOrEqual(targetBeg, targetFin)>
          val Token(colKind, _, _, _, _) = toks(cursor)
          cursor = cursor + 1
          if colKind == TokColon && cursor < endTok then
            val TypeSpan(_, _, elemBeg, elemFin, afterTrait) = Stigi.Emit.parseTypeSpan(tokCap, toks, cursor, endTok)
            targetBeg = elemBeg
            targetFin = elemFin
            cursor = if afterTrait <= endTok then afterTrait else endTok
            foundColon = true
          else ()
        if symCount < symCap then
          syms(symCount) := SymEntry(SymGiven, 0, nameBeg, nameFin, targetBeg, targetFin, 0, 0, 0, 0)
          symCount = symCount + 1
          val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.indexMembers(srcCap, src, tokCap, toks, cursor, endTok, SymGiven, ownerIdx, symCap, syms, symCount, fieldCap, fields, fieldCount)
          if nextSyms <= symCap then symCount = nextSyms else ()
          if nextFields <= fieldCap then fieldCount = nextFields else ()
        else ()
    else ()
  else ()
  IndexCounts(symCount, fieldCount, endTok)
