// prompt: step the cursor over every newline token before the next item.
def Stigi.Parse.passNewlines {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, remaining: U64) <lessOrEqual(tokCount, tokCap), decreases(remaining)>: Unit =
  if remaining > 0 && Stigi.Parse.takeKind{cursor}(tokCap, toks, tokCount, TokNewline) then Stigi.Parse.passNewlines{cursor}(tokCap, toks, tokCount, remaining - 1) else ()
