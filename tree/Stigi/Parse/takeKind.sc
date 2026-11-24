// prompt: consume the token at the cursor when it is of one kind and tell whether it was.
def Stigi.Parse.takeKind {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, want: Stigi.Lex.TokenKind) <lessOrEqual(tokCount, tokCap)>: Bool =
  if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == want then
    cursor.stepForward()
    true
  else false
