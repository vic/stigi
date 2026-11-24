// prompt: the declaration lead of a header keyword token kind.
def Stigi.Parse.leadOfDecl(kind: Stigi.Lex.TokenKind): Stigi.Parse.DeclLead =
  case TokDef => PlainDecl(DeclDef)
  case TokVal => PlainDecl(DeclVal)
  case TokEnum => PlainDecl(DeclEnum)
  case TokType => PlainDecl(DeclType)
  case TokTrait => PlainDecl(DeclTrait)
  case TokGiven => PlainDecl(DeclGiven)
  case TokCapability => PlainDecl(DeclCapability)
  case TokHandler => PlainDecl(DeclHandler)
  case TokRelation => PlainDecl(DeclRelation)
  case TokLemma => DefDecl(DeclLemma)
  case TokRule => DefDecl(DeclRule)
  case TokExtern => ExternDecl
  case TokVar | TokCase | TokMatch | TokIf | TokThen | TokElse | TokWhile | TokLoopBody | TokHandle | TokPub | TokFor | TokState | TokStart | TokStop | TokTrue | TokFalse | TokIs | TokIdent | TokUpper | TokInt | TokStr | TokChar | TokPipe | TokArrow | TokColonAssign | TokEqual | TokNotEqual | TokLessEqual | TokGreatEqual | TokLogicAnd | TokLogicOr | TokBar | TokLess | TokGreat | TokPlus | TokMinus | TokStar | TokSlash | TokPercent | TokAssign | TokColon | TokComma | TokDot | TokDotDot | TokLeftParen | TokRightParen | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokBang | TokNewline | TokEnd => NotDecl
