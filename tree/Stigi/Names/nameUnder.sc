// prompt: whether the name of a unit entry starts with the run of the name pool up to a namespace end and a dot behind it.
def Stigi.Names.nameUnder(pool: Array[U8](Stigi.Limit.maxIndexBytes), from: Stigi.Names.Entry, targetFirst: U64, spaceEnd: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  val Entry(fromFirst, fromEnd, _, _) = from
  if targetFirst < spaceEnd && spaceEnd <= Stigi.Limit.maxIndexBytes then
    val length: U64 = spaceEnd - targetFirst
    val dotAt: U64 = fromFirst + length
    if length < fromEnd - fromFirst && Stigi.Names.poolSame(pool, fromFirst, targetFirst, length) then
      if dotAt < Stigi.Limit.maxIndexBytes then pool(dotAt) == 46 else false
    else false
  else false
