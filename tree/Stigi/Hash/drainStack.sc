// prompt: drain all remaining chaining values from the BLAKE3 Merkle stack into the root digest.
def Stigi.Hash.drainStack {Stigi.Mut} (stackWords: Array[U32](512), initialDepth: U64, chainWords: Array[U32](8), leftChain: Array[U32](8), initChain: Array[U32](8), message: Array[U32](16), scratch: Array[U32](16), state: Array[U32](16)) <lessOrEqual(initialDepth, 60)>: Unit =
  var step: U64 = 0
  while step < initialDepth do
    @<lessOrEqual(step, initialDepth), decreases(initialDepth - step)>
    val nextStep: U64 = step + 1
    val popIndex: U64 = initialDepth - nextStep
    Stigi.Hash.copyFromStack(stackWords, popIndex, leftChain)
    val finalMerge: Bool = popIndex == 0
    Stigi.Hash.compressParent(leftChain, chainWords, finalMerge, initChain, message, scratch, state, chainWords)
    step = nextStep
