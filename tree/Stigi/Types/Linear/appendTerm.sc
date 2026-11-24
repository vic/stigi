// prompt: append one linear term into the pool when space remains and return the updated cursor.
def Stigi.Types.Linear.appendTerm {Stigi.Mut} (poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), outPos: U64, termItem: Stigi.Types.Linear.Term) <lessOrEqual(outPos, poolCap)>: U64 <lessOrEqual(outPos, result), lessOrEqual(result, poolCap)> =
  if outPos < poolCap then
    val nextOut: U64 = outPos + 1
    pool(outPos) := termItem
    nextOut
  else
    outPos
