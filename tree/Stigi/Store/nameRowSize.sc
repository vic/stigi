// prompt: the bytes putNameRow writes: the name and a newline.
def Stigi.Store.nameRowSize(first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: U64 =
  end - first + 1
