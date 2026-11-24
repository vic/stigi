// prompt: the nesting role of one token kind in a parenthesized run.
def Stigi.Parse.nestingOf(kind: Stigi.Lex.TokenKind): Stigi.Parse.Nesting =
  case TokLeftParen => Opens
  case TokRightParen => Closes
  case TokComma => Separates
  case TokEnd => Ends
  case TokDef | TokVal | TokVar | TokEnum | TokCase | TokMatch | TokIf | TokThen | TokElse | TokWhile | TokLoopBody | TokType | TokTrait | TokGiven | TokCapability | TokHandler | TokHandle | TokExtern | TokRelation | TokRule | TokLemma | TokPub | TokFor | TokState | TokStart | TokStop | TokTrue | TokFalse | TokIs | TokIdent | TokUpper | TokInt | TokStr | TokChar | TokPipe | TokArrow | TokColonAssign | TokEqual | TokNotEqual | TokLessEqual | TokGreatEqual | TokLogicAnd | TokLogicOr | TokBar | TokLess | TokGreat | TokPlus | TokMinus | TokStar | TokSlash | TokPercent | TokAssign | TokColon | TokDot | TokDotDot | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokBang | TokNewline => Inside
