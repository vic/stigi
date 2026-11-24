// prompt: the hash of the full name that a candidate stands for, equal to the hash of that name stored in the pool.
def Stigi.Names.candidateHash(pool: Array[U8](Stigi.Limit.maxIndexBytes), tables: Stigi.Syntax.Tables, candidate: Stigi.Names.Candidate) <below(0, Stigi.Limit.maxUnitBytes)>: U64 =
  val Tables(source, _, _, _, _, _) = tables
  candidate match
    case Bare(first, end) => Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, first, end, false, Stigi.Names.hashSeed, end - first + 1)
    case Prefixed(prefixFirst, prefixEnd, first, end) =>
      val prefixHash: U64 = Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, prefixFirst, prefixEnd, false, Stigi.Names.hashSeed, prefixEnd - prefixFirst + 1)
      val dotHash: U64 = U64.wrappingMultiply(U64.bitwiseXor(prefixHash, 46), 1099511628211)
      Stigi.Names.textHash(Stigi.Limit.maxUnitBytes, source, first, end, false, dotHash, end - first + 1)
    case Joined(targetFirst, targetEnd, tailFirst, tailEnd) => Stigi.Names.hashJoined(tables, Stigi.Names.hashSeed, targetFirst, targetEnd, tailFirst, tailEnd)
    case PrefixedJoined(prefixFirst, prefixEnd, targetFirst, targetEnd, tailFirst, tailEnd) =>
      val prefixHash: U64 = Stigi.Names.textHash(Stigi.Limit.maxIndexBytes, pool, prefixFirst, prefixEnd, false, Stigi.Names.hashSeed, prefixEnd - prefixFirst + 1)
      Stigi.Names.hashJoined(tables, U64.wrappingMultiply(U64.bitwiseXor(prefixHash, 46), 1099511628211), targetFirst, targetEnd, tailFirst, tailEnd)
