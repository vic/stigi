// prompt: the slot of the name table where the probe for a candidate name starts, the hash of the full name that the candidate stands for reduced to the table size.
def Stigi.Names.candidateSlot(pool: Array[U8](Stigi.Limit.maxIndexBytes), tables: Stigi.Syntax.Tables, candidate: Stigi.Names.Candidate): U64 <below(result, Stigi.Names.tableSlots)> =
  val Tables(source, _, _, _, _, _) = tables
  val hash: U64 = candidate match
    case Bare(first, end) => Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, first, end, false, Stigi.Names.hashSeed, end - first + 1)
    case Prefixed(prefixFirst, prefixEnd, first, end) =>
      val prefixHash: U64 = Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, prefixFirst, prefixEnd, false, Stigi.Names.hashSeed, prefixEnd - prefixFirst + 1)
      val dotHash: U64 = Stigi.Names.hashStep(prefixHash, U64.ofU8('.'))
      Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, first, end, false, dotHash, end - first + 1)
    case Joined(targetFirst, targetEnd, tailFirst, tailEnd) => Stigi.Names.hashJoined(tables, Stigi.Names.hashSeed, targetFirst, targetEnd, tailFirst, tailEnd)
    case PrefixedJoined(prefixFirst, prefixEnd, targetFirst, targetEnd, tailFirst, tailEnd) =>
      val prefixHash: U64 = Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, prefixFirst, prefixEnd, false, Stigi.Names.hashSeed, prefixEnd - prefixFirst + 1)
      Stigi.Names.hashJoined(tables, Stigi.Names.hashStep(prefixHash, U64.ofU8('.')), targetFirst, targetEnd, tailFirst, tailEnd)
  hash % Stigi.Names.tableSlots
