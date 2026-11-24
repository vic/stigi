// prompt: the test kind a val header becomes when a capability token names Test or NegTest.
def Stigi.Parse.valKind(srcCap: Stigi.ByteCap, src: Array[U8](srcCap), kind: Stigi.Ast.DeclKind, tokKind: Stigi.Lex.TokenKind, segBeg: U64, segFin: U64) <lessOrEqual(segBeg, segFin)>: Stigi.Ast.DeclKind =
  if kind is DeclVal && tokKind is TokUpper && segFin <= srcCap then
    if Stigi.Lex.sliceEquals(srcCap, src, segBeg, segFin, 4, "Test") then DeclTest else if Stigi.Lex.sliceEquals(srcCap, src, segBeg, segFin, 7, "NegTest") then DeclNegTest else kind
  else kind
