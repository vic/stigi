// prompt: tell whether the byte at a position is the closing quote that ends the text exactly.
def Stigi.Syntax.closesAt(srcCap: U64, source: Array[U8](srcCap), closeAt: U64, endPos: U64) <lessOrEqual(endPos, srcCap)>: Bool =
  if closeAt < endPos then
    if U64.ofU8(source(closeAt)) == 39 then closeAt + 1 == endPos else false
  else false
