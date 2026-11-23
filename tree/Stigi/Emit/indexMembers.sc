// prompt: index nested member declarations inside enum, capability, handler, and given units.
def Stigi.Emit.indexMembers {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, parentKind: Stigi.Emit.SymKind, ownerIdx: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), initSyms: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), initFields: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(initSyms, symCap), lessOrEqual(initFields, fieldCap)>: Stigi.Emit.IndexCounts =
  var pos: U64 = startTok
  var symCount: U64 = initSyms
  var fieldCount: U64 = initFields
  var memberCol: U64 = 0
  var hasMemberCol: Bool = false
  while pos < endTok do
    @<lessOrEqual(pos, endTok), lessOrEqual(symCount, symCap), lessOrEqual(fieldCount, fieldCap)>
    val Token(curKind, _, _, _, curCol) = toks(pos)
    if curKind == TokNewline || curKind == TokEnd then
      pos = pos + 1
    else if parentKind == SymEnum && curKind == TokCase && pos + 1 < endTok then
      val namePos: U64 = pos + 1
      val Token(_, caseBeg, caseFin, _, _) = toks(namePos)
      val subStart: U64 = fieldCount
      val IndexCounts(_, nextFields, afterPos) = Stigi.Emit.parseParamList(tokCap, toks, namePos + 1, endTok, FieldEnum, fieldCap, fields, fieldCount)
      if nextFields <= fieldCap then fieldCount = nextFields else ()
      val subCount: U64 = if subStart <= fieldCount then fieldCount - subStart else 0
      if symCount < symCap then
        syms(symCount) := SymEntry(SymCase, ownerIdx, caseBeg, caseFin, 0, 0, subStart, subCount, 0, 0)
        symCount = symCount + 1
      else ()
      pos = if afterPos > pos && afterPos <= endTok then afterPos else pos + 1
    else if curKind == TokDef && (!hasMemberCol || curCol == memberCol) then
      if !hasMemberCol then
        memberCol = curCol
        hasMemberCol = true
      else ()
      val sigBeg: U64 = pos + 1
      var methodEnd: U64 = sigBeg
      var scanDone: Bool = false
      while methodEnd < endTok && !scanDone do
        @<lessOrEqual(sigBeg, methodEnd), lessOrEqual(methodEnd, endTok)>
        val Token(scanKind, _, _, _, scanCol) = toks(methodEnd)
        if scanKind == TokDef && scanCol == memberCol then scanDone = true else if scanKind == TokEnd then scanDone = true else methodEnd = methodEnd + 1
      val memberKind: Stigi.Emit.SymKind = parentKind match
        case SymCap => SymCapMethod
        case SymHandler => SymHandlerMethod
        case SymGiven => SymGivenMethod
        case SymEnum => SymFunc
        case SymCase => SymFunc
        case SymCapMethod => SymFunc
        case SymHandlerMethod => SymFunc
        case SymGivenMethod => SymFunc
        case SymFunc => SymFunc
        case SymGenericFunc => SymFunc
        case SymExternBody => SymFunc
        case SymExternAbi => SymFunc
        case SymTest => SymFunc
        case SymVal => SymFunc
        case SymTypeAlias => SymFunc
      val IndexCounts(nextSyms, nextFields, _) = Stigi.Emit.parseDefSig(srcCap, src, tokCap, toks, sigBeg, methodEnd, memberKind, ownerIdx, symCap, syms, symCount, fieldCap, fields, fieldCount)
      if nextSyms <= symCap then symCount = nextSyms else ()
      if nextFields <= fieldCap then fieldCount = nextFields else ()
      pos = methodEnd
    else if parentKind == SymHandler && curKind == TokVar && (!hasMemberCol || curCol == memberCol) then
      if !hasMemberCol then
        memberCol = curCol
        hasMemberCol = true
      else ()
      val varNamePos: U64 = pos + 1
      val varTypePos: U64 = pos + 3
      if varNamePos < endTok && varTypePos < endTok then
        val Token(_, varBeg, varFin, _, _) = toks(varNamePos)
        val TypeSpan(typeBeg, typeFin, _, _, afterType) = Stigi.Emit.parseTypeSpan(tokCap, toks, varTypePos, endTok)
        var scanPos: U64 = if afterType > pos && afterType <= endTok then afterType else pos + 1
        var initBeg: U64 = 0
        var initFin: U64 = 0
        val valPos: U64 = scanPos + 1
        if scanPos < endTok && valPos < endTok then
          val Token(assignTok, _, _, _, _) = toks(scanPos)
          if assignTok == TokAssign then
            val Token(_, valBeg, valFin, _, _) = toks(valPos)
            initBeg = valBeg
            initFin = valFin
            scanPos = valPos + 1
          else ()
        else ()
        if fieldCount < fieldCap then
          fields(fieldCount) := FieldInfo(FieldStateVar, varBeg, varFin, typeBeg, typeFin, initBeg, initFin)
          fieldCount = fieldCount + 1
        else ()
        pos = scanPos
      else
        pos = pos + 1
    else
      pos = pos + 1
  IndexCounts(symCount, fieldCount, pos)
