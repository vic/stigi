// prompt: strict cursor bound implies inclusive length bound.
pub lemma def Stigi.Lex.cursorWithinLength(cursor: U64, length: U64) <below(cursor, length)>: <lessOrEqual(cursor, length)> =
  @<lessOrEqual(cursor, length)>
