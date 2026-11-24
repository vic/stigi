// prompt: whether a byte run of the name pool equals the join of two runs of the unit text.
def Stigi.Names.sameJoined(pool: Array[U8](Stigi.Limit.maxIndexBytes), at: U64, tables: Stigi.Syntax.Tables, targetFirst: U64, targetEnd: U64, tailFirst: U64, tailEnd: U64) <lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(tailFirst, tailEnd), lessOrEqual(tailEnd, Stigi.Limit.maxUnitBytes)>: Bool =
  val targetLength: U64 = targetEnd - targetFirst
  if at < Stigi.Limit.maxIndexBytes && targetLength < Stigi.Limit.maxIndexBytes - at then Stigi.Names.poolHasText(pool, at, tables, targetFirst, targetEnd, targetLength + 1) && Stigi.Names.poolHasText(pool, at + targetLength, tables, tailFirst, tailEnd, tailEnd - tailFirst + 1) else false
