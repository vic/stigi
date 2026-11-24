// prompt: hash the chunks of an input from a tree walk position, merging and stacking subtrees.
def Stigi.Hash.hashChunks {Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), length: U64, work: Stigi.Hash.Work, tree: Stigi.Hash.Tree, remaining: U64) <lessOrEqual(length, cap), decreases(remaining)>: Stigi.Hash.TreeOutcome =
  val Tree(pos, chunkIndex, depth) = tree
  if remaining > 0 && pos <= length && depth <= 56 && chunkIndex < Stigi.Limit.maxSourceBytes then
    val Work(chain, _, _, _, _, stack, _) = work
    val rest: U64 = length - pos
    val chunkLen: U64 = if rest < Stigi.Hash.chunkBytes then rest else Stigi.Hash.chunkBytes
    Stigi.Hash.resetChain(chain)
    Stigi.Hash.chunkBlocks(cap, data, Chunk(pos, chunkIndex, chunkIndex == 0 && chunkLen == rest), work, pos, chunkLen)
    val nextPos: U64 = pos + chunkLen
    val final: Bool = length <= nextPos
    val merged: U64 = Stigi.Hash.mergeParents(work, depth, chunkIndex + 1, final, 56)
    if final then TreeBuilt else
      if merged < 56 then
        val base: U64 = merged * 8
        if base <= 504 then
          Stigi.Array.copy(8, chain, 0, 8, 512, stack, base)
          Stigi.Hash.hashChunks(cap, data, length, work, Tree(nextPos, chunkIndex + 1, merged + 1), remaining - 1)
        else TreeTooDeep
      else TreeTooDeep
  else TreeTooDeep
