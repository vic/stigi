// prompt: merge completed left subtrees on the BLAKE3 Merkle stack after one non-final chunk.
def Stigi.Hash.mergeSubtrees {Stigi.Mut} (stackWords: Array[U32](512), initialDepth: U64, totalChunks: U64, chainWords: Array[U32](8), leftChain: Array[U32](8), initChain: Array[U32](8), message: Array[U32](16), scratch: Array[U32](16), state: Array[U32](16)) <lessOrEqual(initialDepth, 60)>: U64 <lessOrEqual(result, 60)> =
  var mergeCount: U64 = totalChunks
  var merged: U64 = 0
  while U64.bitwiseAnd(mergeCount, 1) == 0 && merged < initialDepth do
    @<lessOrEqual(merged, initialDepth), decreases(initialDepth - merged)>
    val nextMerged: U64 = merged + 1
    val popIndex: U64 = initialDepth - nextMerged
    Stigi.Hash.copyFromStack(stackWords, popIndex, leftChain)
    Stigi.Hash.compressParent(leftChain, chainWords, false, initChain, message, scratch, state, chainWords)
    mergeCount = U64.shiftRight(mergeCount, 1)
    merged = nextMerged
  val remaining: U64 = initialDepth - merged
  if remaining < 60 then
    Stigi.Hash.copyToStack(stackWords, remaining, chainWords)
    remaining + 1
  else
    remaining
