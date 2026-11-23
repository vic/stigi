// prompt: locate the rightmost lowest-precedence binary or pipe operator at depth zero in a token slice.
def Stigi.Emit.findTopOper(tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap)>: Stigi.Emit.OperSplit =
  var pos: U64 = startTok
  var parenDepth: U64 = 0
  var brackDepth: U64 = 0
  var braceDepth: U64 = 0
  var bestPrec: U64 = 99
  var bestPos: U64 = startTok
  var bestKind: Stigi.Lex.TokenKind = TokEnd
  var foundAny: Bool = false
  var prevWasOper: Bool = true
  while pos < endTok do
    @<lessOrEqual(startTok, pos), lessOrEqual(pos, endTok), lessOrEqual(parenDepth, Stigi.Limit.maxTokens), lessOrEqual(brackDepth, Stigi.Limit.maxTokens), lessOrEqual(braceDepth, Stigi.Limit.maxTokens), lessOrEqual(startTok, bestPos), lessOrEqual(bestPos, endTok)>
    val Token(kind, _, _, _, _) = toks(pos)
    if kind == TokLeftParen && parenDepth < Stigi.Limit.maxTokens then
      parenDepth = parenDepth + 1
      prevWasOper = true
    else if kind == TokRightParen && parenDepth > 0 then
      parenDepth = parenDepth - 1
      prevWasOper = false
    else if kind == TokLeftBrack && brackDepth < Stigi.Limit.maxTokens then
      brackDepth = brackDepth + 1
      prevWasOper = true
    else if kind == TokRightBrack && brackDepth > 0 then
      brackDepth = brackDepth - 1
      prevWasOper = false
    else if kind == TokLeftBrace && braceDepth < Stigi.Limit.maxTokens then
      braceDepth = braceDepth + 1
      prevWasOper = true
    else if kind == TokRightBrace && braceDepth > 0 then
      braceDepth = braceDepth - 1
      prevWasOper = false
    else if parenDepth == 0 && brackDepth == 0 && braceDepth == 0 then
      val prec: U64 = kind match
        case TokPipe => 1
        case TokLogicOr => 2
        case TokLogicAnd => 3
        case TokEqual => 4
        case TokNotEqual => 4
        case TokLess => 5
        case TokLessEqual => 5
        case TokGreat => 5
        case TokGreatEqual => 5
        case TokPlus => 6
        case TokMinus => if prevWasOper then 0 else 6
        case TokStar => 7
        case TokSlash => 7
        case TokPercent => 7
        case TokDef => 0
        case TokVal => 0
        case TokVar => 0
        case TokEnum => 0
        case TokCase => 0
        case TokMatch => 0
        case TokIf => 0
        case TokThen => 0
        case TokElse => 0
        case TokWhile => 0
        case TokLoopBody => 0
        case TokType => 0
        case TokTrait => 0
        case TokGiven => 0
        case TokCapability => 0
        case TokHandler => 0
        case TokHandle => 0
        case TokExtern => 0
        case TokRelation => 0
        case TokRule => 0
        case TokLemma => 0
        case TokPub => 0
        case TokFor => 0
        case TokState => 0
        case TokStart => 0
        case TokStop => 0
        case TokTrue => 0
        case TokFalse => 0
        case TokIs => 0
        case TokIdent => 0
        case TokUpper => 0
        case TokInt => 0
        case TokStr => 0
        case TokChar => 0
        case TokArrow => 0
        case TokColonAssign => 0
        case TokBar => 0
        case TokAssign => 0
        case TokColon => 0
        case TokComma => 0
        case TokDot => 0
        case TokDotDot => 0
        case TokLeftParen => 0
        case TokRightParen => 0
        case TokLeftBrack => 0
        case TokRightBrack => 0
        case TokLeftBrace => 0
        case TokRightBrace => 0
        case TokAt => 0
        case TokBang => 0
        case TokNewline => 0
        case TokEnd => 0
      if prec > 0 then
        if prec <= bestPrec then
          bestPrec = prec
          bestPos = pos
          bestKind = kind
          foundAny = true
        else ()
        prevWasOper = true
      else if kind == TokBang || kind == TokIf || kind == TokThen || kind == TokElse then
        prevWasOper = true
      else
        prevWasOper = false
    else ()
    pos = pos + 1
  if foundAny then OperFound(bestPos, bestKind) else OperNone
