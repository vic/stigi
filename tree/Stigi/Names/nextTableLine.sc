// prompt: the offset of the line that follows the line which starts at an offset of a closed name table.
def Stigi.Names.nextTableLine(size: U64, names: Array[U8](size), at: U64, remaining: U64) <decreases(remaining)>: U64 =
  if remaining < 1 || size <= at then size else
    if names(at) == 10 then at + 1 else Stigi.Names.nextTableLine(size, names, at + 1, remaining - 1)
