// prompt: the offset of the line that follows the line which starts at an offset of the builtin text.
def Stigi.Names.nextLine(names: Array[U8](1171), at: U64, remaining: U64) <decreases(remaining)>: U64 =
  if remaining < 1 || 1171 <= at then 1171 else
    if names(at) == 10 then at + 1 else Stigi.Names.nextLine(names, at + 1, remaining - 1)
