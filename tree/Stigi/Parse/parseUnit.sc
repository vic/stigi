// prompt: parse a complete top-level L0 Stigi unit header and root body expression.
pub def Stigi.Parse.parseUnit {Stigi.Mut} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Ast.ParseResult =
  handle cursor = Stigi.Parse.TokenCursor(tokCap, nodeCap)
  Stigi.Parse.passNewlines{cursor}(tokCap, toks, tokCount, tokCount)
  val isPublic: Bool = Stigi.Parse.takeKind{cursor}(tokCap, toks, tokCount, TokPub)
  val Started(leadKind) = Stigi.Parse.startHeader{cursor}(tokCap, toks, tokCount) else
    case NotStarted => ParseFailed(cursor.curPos())
  val Named(nameStart, nameEnd) = Stigi.Parse.parseName{cursor}(tokCap, toks, tokCount) else
    case Unnamed => ParseFailed(cursor.curPos())
  val BodyAt(declKind, bodyStart) = Stigi.Parse.findBody{cursor}(srcCap, src, tokCap, toks, tokCount, leadKind, tokCount)
  Stigi.Parse.passNewlines{cursor}(tokCap, toks, tokCount, tokCount)
  val bodyEnd: U64 = if bodyStart <= tokCount then tokCount else bodyStart
  if Stigi.Parse.hasExprBody(declKind, Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)) then
    Stigi.Parse.parseExpr{cursor}(tokCap, toks, tokCount, nodeCap, nodes) match
      case Found(rootIdx) => ParsedUnit(UnitHeader(declKind, isPublic, nameStart, nameEnd, bodyStart, bodyEnd, ExprRoot(rootIdx)), cursor.usedNodes())
      case Missing => ParseFailed(cursor.curPos())
  else ParsedUnit(UnitHeader(declKind, isPublic, nameStart, nameEnd, bodyStart, bodyEnd, NonExprBody), cursor.usedNodes())
