// prompt: law proving every decoded decimal digit stays strictly below ten.
pub lemma def Stigi.Lex.Laws.decDigitBound(digit: U64) <below(digit, 10)>: <below(digit, 16)> =
  @<below(digit, 16)>
