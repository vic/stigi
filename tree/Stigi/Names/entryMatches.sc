// prompt: whether one image entry carries the name that a candidate stands for.
def Stigi.Names.entryMatches(pool: Array[U8](Stigi.Limit.maxIndexBytes), entry: Stigi.Names.Entry, tables: Stigi.Syntax.Tables, candidate: Stigi.Names.Candidate) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  val Entry(entryFirst, entryEnd, _, _) = entry
  candidate match
    case Bare(first, end) => entryEnd - entryFirst == end - first && Stigi.Names.poolHasText(pool, entryFirst, tables, first, end, end - first + 1)
    case Prefixed(prefixFirst, prefixEnd, first, end) =>
      val prefixLength: U64 = prefixEnd - prefixFirst
      val dotAt: U64 = entryFirst + prefixLength
      if entryEnd - entryFirst == prefixLength + 1 + (end - first) && Stigi.Names.poolSame(pool, entryFirst, prefixFirst, prefixLength) then
        if dotAt < Stigi.Limit.maxIndexBytes then pool(dotAt) == 46 && Stigi.Names.poolHasText(pool, dotAt + 1, tables, first, end, end - first + 1) else false
      else false
    case Joined(targetFirst, targetEnd, tailFirst, tailEnd) => entryEnd - entryFirst == targetEnd - targetFirst + (tailEnd - tailFirst) && Stigi.Names.sameJoined(pool, entryFirst, tables, targetFirst, targetEnd, tailFirst, tailEnd)
    case PrefixedJoined(prefixFirst, prefixEnd, targetFirst, targetEnd, tailFirst, tailEnd) =>
      val prefixLength: U64 = prefixEnd - prefixFirst
      val dotAt: U64 = entryFirst + prefixLength
      if entryEnd - entryFirst == prefixLength + 1 + (targetEnd - targetFirst) + (tailEnd - tailFirst) && Stigi.Names.poolSame(pool, entryFirst, prefixFirst, prefixLength) then
        if dotAt < Stigi.Limit.maxIndexBytes then pool(dotAt) == 46 && Stigi.Names.sameJoined(pool, dotAt + 1, tables, targetFirst, targetEnd, tailFirst, tailEnd) else false
      else false
