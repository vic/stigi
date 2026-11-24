// prompt: the expression lead of the first token kind of an expression.
def Stigi.Parse.leadOfExpr(kind: Stigi.Lex.TokenKind): Stigi.Parse.ExprLead =
  case TokIf => LeadHead(ExprIf)
  case TokWhile => LeadHead(ExprWhile)
  case TokVal => LeadBind(ExprVal)
  case TokVar => LeadBind(ExprVar)
  case TokHandle => LeadHandle
  case TokDef | TokEnum | TokCase | TokMatch | TokThen | TokElse | TokLoopBody | TokType | TokTrait | TokGiven | TokCapability | TokHandler | TokExtern | TokRelation | TokRule | TokLemma | TokPub | TokFor | TokState | TokStart | TokStop | TokTrue | TokFalse | TokIs | TokIdent | TokUpper | TokInt | TokStr | TokChar | TokPipe | TokArrow | TokColonAssign | TokEqual | TokNotEqual | TokLessEqual | TokGreatEqual | TokLogicAnd | TokLogicOr | TokBar | TokLess | TokGreat | TokPlus | TokMinus | TokStar | TokSlash | TokPercent | TokAssign | TokColon | TokComma | TokDot | TokDotDot | TokLeftParen | TokRightParen | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokBang | TokNewline | TokEnd => LeadOperand
