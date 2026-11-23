// prompt: parse a def or method signature (generic param, capabilities, params, return type, and body span).
def Stigi.Emit.parseDefSig {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, baseKind: Stigi.Emit.SymKind, ownerIdx: U64, symCap: Stigi.NodeCap, syms: Array[Stigi.Emit.SymEntry](symCap), initSyms: U64, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), initFields: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(initSyms, symCap), lessOrEqual(initFields, fieldCap)>: Stigi.Emit.IndexCounts =
  var pos: U64 = startTok
  var symCount: U64 = initSyms
  var fieldCount: U64 = initFields
  if pos >= endTok then IndexCounts(symCount, fieldCount, pos) else
    val Token(_, nameBeg, firstFin, _, _) = toks(pos)
    var nameEnd: U64 = firstFin
    pos = pos + 1
    var dotGuard: U64 = 0
    while pos < endTok && dotGuard < 16 do
      @<lessOrEqual(pos, endTok), lessOrEqual(nameBeg, nameEnd), lessOrEqual(dotGuard, 16)>
      val Token(dotKind, _, _, _, _) = toks(pos)
      if dotKind == TokDot && pos + 1 < endTok then
        val segPos: U64 = pos + 1
        val Token(_, _, segFin, _, _) = toks(segPos)
        if nameBeg <= segFin then nameEnd = segFin else ()
        pos = segPos + 1
        dotGuard = dotGuard + 1
      else
        dotGuard = 16
    var isGeneric: Bool = false
    if pos < endTok then
      val Token(genKind, _, _, _, _) = toks(pos)
      if genKind == TokLeftBrack then
        isGeneric = true
        pos = pos + 1
        var brackDone: Bool = false
        while pos < endTok && !brackDone do
          @<lessOrEqual(pos, endTok)>
          val Token(curKind, _, _, _, _) = toks(pos)
          pos = pos + 1
          if curKind == TokRightBrack then brackDone = true else ()
      else ()
    else ()
    val subStart: U64 = fieldCount
    if pos < endTok then
      val Token(braceKind, _, _, _, _) = toks(pos)
      if braceKind == TokLeftBrace then
        pos = pos + 1
        var braceDone: Bool = false
        while pos < endTok && !braceDone do
          @<lessOrEqual(pos, endTok), lessOrEqual(fieldCount, fieldCap)>
          val Token(curKind, capNameBeg, capNameFin, _, _) = toks(pos)
          if curKind == TokRightBrace then
            pos = pos + 1
            braceDone = true
          else if curKind == TokComma then
            pos = pos + 1
          else
            val nextPos: U64 = pos + 1
            val typeStartPos: U64 = nextPos + 1
            if nextPos < endTok && typeStartPos < endTok then
              val Token(colonKind, _, _, _, _) = toks(nextPos)
              if colonKind == TokColon then
                val TypeSpan(capTypeBeg, capTypeFin, capElemBeg, capElemFin, afterCap) = Stigi.Emit.parseTypeSpan(tokCap, toks, typeStartPos, endTok)
                val isRegion: Bool = if capTypeFin <= srcCap then Stigi.Lex.sliceEquals(srcCap, src, capTypeBeg, capTypeFin, 12, "Stigi.Region") else false
                if !isRegion && fieldCount < fieldCap then
                  fields(fieldCount) := FieldInfo(FieldCapParam, capNameBeg, capNameFin, capTypeBeg, capTypeFin, capElemBeg, capElemFin)
                  fieldCount = fieldCount + 1
                else ()
                pos = if afterCap > pos && afterCap <= endTok then afterCap else pos + 1
              else
                val TypeSpan(_, _, _, _, afterMarker) = Stigi.Emit.parseTypeSpan(tokCap, toks, pos, endTok)
                pos = if afterMarker > pos && afterMarker <= endTok then afterMarker else pos + 1
            else
              pos = pos + 1
      else ()
    else ()
    val IndexCounts(_, afterParamsCount, afterParamsPos) = Stigi.Emit.parseParamList(tokCap, toks, pos, endTok, FieldParam, fieldCap, fields, fieldCount)
    if afterParamsCount <= fieldCap then fieldCount = afterParamsCount else ()
    if afterParamsPos <= endTok then pos = afterParamsPos else ()
    pos = Stigi.Emit.passAngles(tokCap, toks, pos, endTok)
    var retStart: U64 = 0
    var retEnd: U64 = 0
    if pos < endTok then
      val Token(retColon, _, _, _, _) = toks(pos)
      if retColon == TokColon then
        pos = pos + 1
        val TypeSpan(retBeg, retFin, _, _, afterRet) = Stigi.Emit.parseTypeSpan(tokCap, toks, pos, endTok)
        retStart = retBeg
        retEnd = retFin
        if afterRet <= endTok then pos = afterRet else ()
      else ()
    else ()
    pos = Stigi.Emit.passAngles(tokCap, toks, pos, endTok)
    var hasBody: Bool = false
    var bodyStart: U64 = pos
    if pos < endTok then
      val Token(assignKind, _, _, _, _) = toks(pos)
      if assignKind == TokAssign then
        hasBody = true
        pos = pos + 1
        bodyStart = pos
      else ()
    else ()
    val finalKind: Stigi.Emit.SymKind = baseKind match
      case SymFunc => if isGeneric then SymGenericFunc else SymFunc
      case SymExternBody => if hasBody then SymExternBody else SymExternAbi
      case SymEnum => SymEnum
      case SymCase => SymCase
      case SymCap => SymCap
      case SymCapMethod => SymCapMethod
      case SymHandler => SymHandler
      case SymHandlerMethod => SymHandlerMethod
      case SymGiven => SymGiven
      case SymGivenMethod => SymGivenMethod
      case SymGenericFunc => SymGenericFunc
      case SymExternAbi => SymExternAbi
      case SymTest => SymTest
      case SymVal => SymVal
      case SymTypeAlias => SymTypeAlias
    val subCount: U64 = if subStart <= fieldCount then fieldCount - subStart else 0
    val bodyEnd: U64 = if hasBody then endTok else bodyStart
    if symCount < symCap then
      syms(symCount) := SymEntry(finalKind, ownerIdx, nameBeg, nameEnd, retStart, retEnd, subStart, subCount, bodyStart, bodyEnd)
      symCount = symCount + 1
    else ()
    IndexCounts(symCount, fieldCount, pos)
