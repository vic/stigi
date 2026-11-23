// prompt: law proving binary operator precedence stays bounded by any upper limit above six.
pub lemma def Stigi.Parse.Laws.precedenceBound(kind: Stigi.Lex.TokenKind, upperLim: U64) <lessOrEqual(6, upperLim)>: <lessOrEqual(6, upperLim)> =
  val prec: U64 = Stigi.Parse.binPrecedence(kind)
  @<lessOrEqual(prec, upperLim)>
