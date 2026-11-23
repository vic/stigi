// prompt: law proving every decoded decimal digit stays strictly below ten.
pub lemma def Stigi.Lex.Laws.decDigitBound(digit: U64) <below(digit, 10)>: <Stigi.Rules.withinLimit(digit, 16)> =
  @<Stigi.Rules.withinLimit(digit, 16)>
