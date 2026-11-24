// prompt: compress the blocks of one chunk from a block start with the bytes still left in the chunk.
def Stigi.Hash.chunkBlocks {Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), chunk: Stigi.Hash.Chunk, work: Stigi.Hash.Work, blockStart: U64, rest: U64) <decreases(rest)>: Unit =
  val Chunk(start, index, root) = chunk
  val takeLen: U64 = if rest < 64 then rest else 64
  val firstFlag: U32 = if blockStart == start then Stigi.Hash.chunkStartFlag else U32.ofU64(0)
  if blockStart <= cap then
    Stigi.Hash.loadBlockWords(cap, data, blockStart, takeLen, work)
    if rest <= 64 then
      val endFlags: U32 = U32.bitwiseOr(firstFlag, Stigi.Hash.chunkEndFlag)
      Stigi.Hash.compressBlock(work, index, U32.ofU64(takeLen), if root then U32.bitwiseOr(endFlags, Stigi.Hash.rootFlag) else endFlags)
    else
      Stigi.Hash.compressBlock(work, index, U32.ofU64(takeLen), firstFlag)
      Stigi.Hash.chunkBlocks(cap, data, chunk, work, blockStart + 64, rest - 64)
  else ()
