// prompt: consume the declaration keywords of a unit header and give its declaration kind.
def Stigi.Parse.startHeader {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64) <lessOrEqual(tokCount, tokCap)>: Stigi.Parse.DeclStart =
  Stigi.Parse.leadOfDecl(Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)) match
    case PlainDecl(kind) =>
      cursor.stepForward()
      Started(kind)
    case DefDecl(kind) =>
      cursor.stepForward()
      if Stigi.Parse.takeKind{cursor}(tokCap, toks, tokCount, TokDef) then Started(kind) else NotStarted
    case ExternDecl =>
      cursor.stepForward()
      if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) is TokStr then cursor.stepForward() else ()
      if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) is TokDef | TokVal then
        cursor.stepForward()
        Started(DeclExtern)
      else NotStarted
    case NotDecl => NotStarted
