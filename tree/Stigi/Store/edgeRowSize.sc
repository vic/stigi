// prompt: the bytes putEdgeRow writes: the first name, a tab, the second name and a newline.
def Stigi.Store.edgeRowSize(fromFirst: U64, fromEnd: U64, toFirst: U64, toEnd: U64) <lessOrEqual(fromFirst, fromEnd), lessOrEqual(fromEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(toFirst, toEnd), lessOrEqual(toEnd, Stigi.Limit.maxIndexBytes)>: U64 =
  fromEnd - fromFirst + (toEnd - toFirst) + 2
