// prompt: the source bytes from the first byte of one token to the last byte of a following token.
def Stigi.Syntax.Parse.spanText(tables: Stigi.Syntax.Tables, first: U64, last: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Text =
  val Token(_, startPos, _, _, _) = Stigi.Syntax.Parse.tokenAt(tables, first)
  val Token(_, _, endPos, _, _) = Stigi.Syntax.Parse.tokenAt(tables, last)
  if startPos <= endPos && endPos <= Stigi.Limit.maxUnitBytes then Text(startPos, endPos) else Text(0, 0)
