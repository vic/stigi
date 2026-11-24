// prompt: parse one declaration at a column: an optional pub, then a def, lemma def, extern, val, enum, type, relation, rule def, capability, handler, trait or given.
def Stigi.Syntax.Parse.parseDecl {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, column: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if depth < 1 then Failed(TooDeep, token) else
    val isPublic: Bool = Stigi.Syntax.Parse.kindAt(tables, token) is TokPub
    val keyword: U64 = if isPublic then Stigi.Syntax.Parse.nextToken(token) else token
    val second: U64 = Stigi.Syntax.Parse.nextToken(keyword)
    val third: U64 = Stigi.Syntax.Parse.nextToken(second)
    val fourth: U64 = Stigi.Syntax.Parse.nextToken(third)
    val atSecond: Stigi.Syntax.Parse.Progress = Progress(second, nodes, lists, pending)
    val atThird: Stigi.Syntax.Parse.Progress = Progress(third, nodes, lists, pending)
    Stigi.Syntax.Parse.kindAt(tables, keyword) match
      case TokDef => Stigi.Syntax.Parse.parseFunction(tables, atSecond, isPublic, PlainFunction, column, depth - 1)
      case TokLemma => Stigi.Syntax.Parse.parseFunction(tables, atThird, isPublic, LemmaFunction, column, depth - 1)
      case TokExtern => if Stigi.Syntax.Parse.kindAt(tables, third) is TokVal then Stigi.Syntax.Parse.readExternVal(tables, Progress(fourth, nodes, lists, pending), isPublic, Stigi.Syntax.Parse.spanText(tables, second, second)) else Stigi.Syntax.Parse.parseFunction(tables, Progress(fourth, nodes, lists, pending), isPublic, ExternFunction(Stigi.Syntax.Parse.spanText(tables, second, second)), column, depth - 1)
      case TokVal => Stigi.Syntax.Parse.parseValDecl(tables, atSecond, isPublic, column, depth - 1)
      case TokEnum => Stigi.Syntax.Parse.readEnum(tables, atSecond, isPublic)
      case TokType => Stigi.Syntax.Parse.readTypeDecl(tables, atSecond, isPublic)
      case TokRelation => Stigi.Syntax.Parse.readRelation(tables, atSecond, isPublic)
      case TokRule => Stigi.Syntax.Parse.readRule(tables, atThird, isPublic)
      case TokCapability => Stigi.Syntax.Parse.parseCompound(tables, atSecond, isPublic, IsCapability, depth - 1)
      case TokHandler => Stigi.Syntax.Parse.parseCompound(tables, atSecond, isPublic, IsHandler, depth - 1)
      case TokTrait => Stigi.Syntax.Parse.parseCompound(tables, atSecond, isPublic, IsTrait, depth - 1)
      case TokGiven => Stigi.Syntax.Parse.parseCompound(tables, atSecond, isPublic, IsGiven, depth - 1)
      case TokVar | TokCase | TokMatch | TokIf | TokThen | TokElse | TokWhile | TokLoopBody | TokHandle | TokPub | TokFor | TokState | TokStart | TokStop | TokTrue | TokFalse | TokIs | TokIdent | TokUpper | TokInt | TokStr | TokChar | TokPipe | TokArrow | TokColonAssign | TokEqual | TokNotEqual | TokLessEqual | TokGreatEqual | TokLogicAnd | TokLogicOr | TokBar | TokLess | TokGreat | TokPlus | TokMinus | TokStar | TokSlash | TokPercent | TokAssign | TokColon | TokComma | TokDot | TokDotDot | TokLeftParen | TokRightParen | TokLeftBrack | TokRightBrack | TokLeftBrace | TokRightBrace | TokAt | TokBang | TokNewline | TokEnd => Failed(UnexpectedToken, keyword)
