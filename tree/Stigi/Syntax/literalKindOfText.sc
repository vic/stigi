// prompt: the kind of a literal written in source, read from the first byte of its own text.
pub def Stigi.Syntax.literalKindOfText(srcCap: U64, source: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(endPos, srcCap)>: Stigi.Syntax.LiteralKind =
  if first < endPos then
    val lead: U64 = U64.ofU8(source(first))
    if lead == 39 then CharLiteral else if lead == 34 then StringLiteral else if lead == 116 then TrueLiteral else if lead == 102 then FalseLiteral else IntLiteral
  else IntLiteral
