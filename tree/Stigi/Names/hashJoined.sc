// prompt: the hash of the join of two runs of the unit text, the second run continuing the accumulator of the first.
def Stigi.Names.hashJoined(tables: Stigi.Syntax.Tables, acc: U64, targetFirst: U64, targetEnd: U64, tailFirst: U64, tailEnd: U64) <lessOrEqual(targetFirst, targetEnd), lessOrEqual(targetEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(tailFirst, tailEnd), lessOrEqual(tailEnd, Stigi.Limit.maxUnitBytes)>: U64 =
  val Tables(source, _, _, _, _, _) = tables
  Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, tailFirst, tailEnd, false, Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, targetFirst, targetEnd, false, acc, targetEnd - targetFirst + 1), tailEnd - tailFirst + 1)
