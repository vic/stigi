// prompt: parse a complete top-level L0 Stigi unit header and root body expression.
pub def Stigi.Parse.parseUnit {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Ast.ParseResult =
  handle cursor = Stigi.Parse.TokenCursor(tokCap, nodeCap)
  var leadGuard: U64 = 0
  while Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokNewline && leadGuard < tokCount do
    @<lessOrEqual(leadGuard, tokCount)>
    cursor.stepForward()
    leadGuard = leadGuard + 1
  val isPublic: Bool =
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokPub then
      cursor.stepForward()
      true
    else false
  val leadKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
  var declKind: Stigi.Ast.DeclKind = DeclDef
  var validHeader: Bool = true
  if leadKind == TokDef then
    declKind = DeclDef
    cursor.stepForward()
  else if leadKind == TokLemma then
    declKind = DeclLemma
    cursor.stepForward()
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokDef then cursor.stepForward() else validHeader = false
  else if leadKind == TokVal then
    declKind = DeclVal
    cursor.stepForward()
  else if leadKind == TokEnum then
    declKind = DeclEnum
    cursor.stepForward()
  else if leadKind == TokType then
    declKind = DeclType
    cursor.stepForward()
  else if leadKind == TokTrait then
    declKind = DeclTrait
    cursor.stepForward()
  else if leadKind == TokGiven then
    declKind = DeclGiven
    cursor.stepForward()
  else if leadKind == TokCapability then
    declKind = DeclCapability
    cursor.stepForward()
  else if leadKind == TokHandler then
    declKind = DeclHandler
    cursor.stepForward()
  else if leadKind == TokRelation then
    declKind = DeclRelation
    cursor.stepForward()
  else if leadKind == TokRule then
    declKind = DeclRule
    cursor.stepForward()
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokDef then cursor.stepForward() else validHeader = false
  else if leadKind == TokExtern then
    declKind = DeclExtern
    cursor.stepForward()
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokStr then cursor.stepForward() else ()
    val extSub: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
    if extSub == TokDef || extSub == TokVal then cursor.stepForward() else validHeader = false
  else
    validHeader = false
  if !validHeader then ParseFailed(cursor.curPos()) else
    val nameTokPos: U64 = cursor.curPos()
    val nameKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
    val isNameTok: Bool = nameKind == TokIdent || nameKind == TokUpper
    if nameTokPos < tokCount && isNameTok then
      val Token(_, nameStart, initEnd, _, _) = toks(nameTokPos)
      var nameEnd: U64 = initEnd
      cursor.stepForward()
      var dotSteps: U64 = 0
      while Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokDot && dotSteps < tokCount do
        @<lessOrEqual(dotSteps, tokCount), lessOrEqual(nameStart, nameEnd)>
        cursor.stepForward()
        val segPos: U64 = cursor.curPos()
        val segKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
        val isSegName: Bool = segKind == TokIdent || segKind == TokUpper
        if segPos < tokCount && isSegName then
          val Token(_, _, segEnd, _, _) = toks(segPos)
          if nameStart <= segEnd then nameEnd = segEnd else ()
          cursor.stepForward()
        else ()
        dotSteps = dotSteps + 1
      var bodyStart: U64 = cursor.curPos()
      var scanLim: U64 = 0
      var foundBody: Bool = false
      while !foundBody && scanLim < tokCount do
        @<lessOrEqual(scanLim, tokCount)>
        val curTok: U64 = cursor.curPos()
        val curKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
        if curKind == TokEnd then
          foundBody = true
        else if curTok < tokCount && (curKind == TokAssign || curKind == TokColon) then
          val afterTok: U64 = curTok + 1
          val isColonDecl: Bool = declKind == DeclEnum || declKind == DeclTrait || declKind == DeclGiven || declKind == DeclCapability || declKind == DeclHandler
          if declKind == DeclVal then
            if curKind == TokAssign then
              cursor.stepForward()
              bodyStart = afterTok
              foundBody = true
            else
              cursor.stepForward()
          else if curKind == TokAssign || isColonDecl then
            cursor.stepForward()
            bodyStart = afterTok
            foundBody = true
          else
            cursor.stepForward()
        else
          if declKind == DeclVal && curKind == TokUpper && curTok < tokCount then
            val Token(_, segBeg, segFin, _, _) = toks(curTok)
            if segFin <= srcCap then
              if Stigi.Lex.sliceEquals(srcCap, src, segBeg, segFin, 4, "Test") then declKind = DeclTest else if Stigi.Lex.sliceEquals(srcCap, src, segBeg, segFin, 7, "NegTest") then declKind = DeclNegTest else ()
            else ()
          else ()
          cursor.stepForward()
        scanLim = scanLim + 1
      var bodyLineGuard: U64 = 0
      while Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokNewline && bodyLineGuard < tokCount do
        @<lessOrEqual(bodyLineGuard, tokCount)>
        cursor.stepForward()
        bodyLineGuard = bodyLineGuard + 1
      val exprKindNow: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
      val hasExprDecl: Bool = declKind == DeclDef || declKind == DeclVal || declKind == DeclLemma || declKind == DeclTest
      val rootIdx: U64 =
        if exprKindNow != TokEnd && hasExprDecl then
          Stigi.Parse.parseExpr{cursor}(tokCap, toks, tokCount, nodeCap, nodes) match
            case Found(nodeIdx) => nodeIdx
            case Missing => 0
        else 0
      val bodyEnd: U64 = if bodyStart <= tokCount then tokCount else bodyStart
      ParsedUnit(UnitHeader(declKind, isPublic, nameStart, nameEnd, bodyStart, bodyEnd, rootIdx), cursor.usedNodes())
    else ParseFailed(nameTokPos)
