// prompt: map an L0 binary operator token kind to its precedence level (zero if not binary).
def Stigi.Parse.binPrecedence(kind: Stigi.Lex.TokenKind): U64 <lessOrEqual(result, 6)> =
  case TokPipe => 1
  case TokLogicOr => 2
  case TokLogicAnd => 3
  case TokEqual | TokNotEqual | TokLess | TokLessEqual | TokGreat | TokGreatEqual => 4
  case TokPlus | TokMinus => 5
  case TokStar | TokSlash | TokPercent => 6
  case TokDef | TokVal | TokVar | TokEnum | TokCase | TokMatch | TokIf | TokThen | TokElse | TokWhile | TokLoopBody | TokType | TokTrait | TokGiven | TokCapability | TokHandler | TokHandle | TokExtern | TokRelation | TokRule | TokLemma | TokPub | TokFor | TokState | TokStart | TokStop | TokTrue | TokFalse | TokIs | TokIdent | TokUpper | TokInt | TokStr | TokChar | TokArrow | TokColonAssign | TokBar | TokAssign | TokColon | TokComma | TokDot | TokDotDot | TokLeftParen | TokRightParen | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokBang | TokNewline | TokEnd => 0
