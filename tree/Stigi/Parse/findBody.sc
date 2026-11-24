// prompt: step over the rest of a header to the token after its body opener.
def Stigi.Parse.findBody {cursor: Stigi.Parse.Cursor} (srcCap: Stigi.ByteCap, src: Array[U8](srcCap), tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, kind: Stigi.Ast.DeclKind, remaining: U64) <lessOrEqual(tokCount, tokCap), decreases(remaining)>: Stigi.Parse.BodyStart =
  val pos: U64 = cursor.curPos()
  if remaining > 0 && pos < tokCount then
    val Token(tokKind, segBeg, segFin, _, _) = toks(pos)
    if tokKind is TokEnd then BodyAt(kind, pos) else
      cursor.stepForward()
      if tokKind is TokAssign || tokKind is TokColon && kind is DeclEnum | DeclTrait | DeclGiven | DeclCapability | DeclHandler then BodyAt(kind, pos + 1) else
        Stigi.Parse.findBody{cursor}(srcCap, src, tokCap, toks, tokCount, Stigi.Parse.valKind(srcCap, src, kind, tokKind, segBeg, segFin), remaining - 1)
  else BodyAt(kind, pos)
