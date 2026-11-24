// prompt: parse a primary or unary expression atom into the flat AST node arena.
def Stigi.Parse.parseAtom {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Lookup =
  val pos: U64 = cursor.curPos()
  Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) match
    case TokInt => Stigi.Parse.leaf{cursor}(nodeCap, nodes, ExprInt)
    case TokTrue | TokFalse => Stigi.Parse.leaf{cursor}(nodeCap, nodes, ExprBool)
    case TokStr => Stigi.Parse.leaf{cursor}(nodeCap, nodes, ExprStr)
    case TokChar => Stigi.Parse.leaf{cursor}(nodeCap, nodes, ExprChar)
    case TokIdent | TokUpper => Stigi.Parse.parsePath{cursor}(tokCap, toks, tokCount, nodeCap, nodes)
    case TokBang | TokMinus => Stigi.Parse.parseUnary{cursor}(tokCap, toks, tokCount, nodeCap, nodes)
    case TokLeftParen =>
      cursor.stepForward()
      val innerPos: U64 = cursor.curPos()
      Stigi.Parse.scanArgs{cursor}(tokCap, toks, tokCount, ArgState(1, 0, 0), tokCount) match
        case ArgsClosed(_, 0) => cursor.emitNode(nodeCap, nodes, Node(ExprUnit, pos, 0, 0, 0, 0))
        case ArgsClosed(_, _) => cursor.emitNode(nodeCap, nodes, Node(ExprBlock, pos, innerPos, cursor.curPos(), 0, 0))
        case ArgsOpen => Missing
    case TokDef | TokVal | TokVar | TokEnum | TokCase | TokMatch | TokIf | TokThen | TokElse | TokWhile | TokLoopBody | TokType | TokTrait | TokGiven | TokCapability | TokHandler | TokHandle | TokExtern | TokRelation | TokRule | TokLemma | TokPub | TokFor | TokState | TokStart | TokStop | TokIs | TokPipe | TokArrow | TokColonAssign | TokEqual | TokNotEqual | TokLessEqual | TokGreatEqual | TokLogicAnd | TokLogicOr | TokBar | TokLess | TokGreat | TokPlus | TokStar | TokSlash | TokPercent | TokAssign | TokColon | TokComma | TokDot | TokDotDot | TokRightParen | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokNewline | TokEnd => Missing
