// prompt: merge stacked BLAKE3 subtrees into parents while the chunk count is even, or all of them after the last chunk.
def Stigi.Hash.mergeParents {Stigi.Mut} (work: Stigi.Hash.Work, depth: U64, total: U64, final: Bool, remaining: U64) <lessOrEqual(depth, 56), decreases(remaining)>: U64 <lessOrEqual(result, 56)> =
  if remaining > 0 && depth > 0 && (final || U64.bitwiseAnd(total, 1) == 0) then
    val parentDepth: U64 = depth - 1
    Stigi.Hash.compressParent(work, parentDepth, final && parentDepth == 0)
    Stigi.Hash.mergeParents(work, parentDepth, U64.shiftRight(total, 1), final, remaining - 1)
  else depth
