// prompt: hash a multi-chunk input longer than 1024 bytes using the BLAKE3 Merkle stack.
def Stigi.Hash.hashMultiChunk {Stigi.Mut} (capacity: U64, input: Array[U8](capacity), length: U64, chainWords: Array[U32](8), message: Array[U32](16), scratch: Array[U32](16), state: Array[U32](16), stackWords: Array[U32](512), leftChain: Array[U32](8), initChain: Array[U32](8)) <below(1024, length), lessOrEqual(length, capacity), below(capacity, 0x100000000)>: Stigi.Hash.Digest =
  var stackDepth: U64 = 0
  var chunkIndex: U64 = 0
  var cursor: U64 = 0
  while cursor < length && chunkIndex < length do
    @<lessOrEqual(cursor, length), lessOrEqual(chunkIndex, length), lessOrEqual(stackDepth, 60), decreases(length - chunkIndex)>
    val plusChunk: U64 = cursor + 1024
    val nextCursor: U64 = if plusChunk < length then plusChunk else length
    Stigi.Hash.compressChunk(capacity, input, cursor, nextCursor, chunkIndex, false, chainWords, message, scratch, state)
    val totalChunks: U64 = chunkIndex + 1
    if nextCursor < length then
      stackDepth = Stigi.Hash.mergeSubtrees(stackWords, stackDepth, totalChunks, chainWords, leftChain, initChain, message, scratch, state)
    else
      ()
    chunkIndex = totalChunks
    cursor = nextCursor
  Stigi.Hash.drainStack(stackWords, stackDepth, chainWords, leftChain, initChain, message, scratch, state)
  Stigi.Hash.Digest.DigestWords(chainWords(0), chainWords(1), chainWords(2), chainWords(3), chainWords(4), chainWords(5), chainWords(6), chainWords(7))
