// prompt: classify the token kind after an operand as a binary operator or as none.
def Stigi.Syntax.Parse.infixOf(kind: Stigi.Lex.TokenKind): Stigi.Syntax.Parse.Infix =
  case TokPipe => Infix(PipeInto)
  case TokLogicOr => Infix(LogicOr)
  case TokLogicAnd => Infix(LogicAnd)
  case TokEqual => Infix(Equals)
  case TokNotEqual => Infix(Differs)
  case TokLess => Infix(LessThan)
  case TokLessEqual => Infix(AtMost)
  case TokGreat => Infix(GreaterThan)
  case TokGreatEqual => Infix(AtLeast)
  case TokPlus => Infix(Plus)
  case TokMinus => Infix(Minus)
  case TokStar => Infix(Times)
  case TokSlash => Infix(Divide)
  case TokPercent => Infix(Remainder)
  case TokDef | TokVal | TokVar | TokEnum | TokCase | TokMatch | TokIf | TokThen | TokElse | TokWhile | TokLoopBody | TokType | TokTrait | TokGiven | TokCapability | TokHandler | TokHandle | TokExtern | TokRelation | TokRule | TokLemma | TokPub | TokFor | TokState | TokStart | TokStop | TokTrue | TokFalse | TokIs | TokIdent | TokUpper | TokInt | TokStr | TokChar | TokArrow | TokColonAssign | TokBar | TokAssign | TokColon | TokComma | TokDot | TokDotDot | TokLeftParen | TokRightParen | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokBang | TokNewline | TokEnd => Standalone
