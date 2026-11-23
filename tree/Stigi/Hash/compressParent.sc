// prompt: compress one BLAKE3 parent node combining a stacked left chain and current right chain.
def Stigi.Hash.compressParent {region: Stigi.Region, Stigi.Mut} (stackWords: Array[U32](512), parentDepth: U64, chain: Array[U32](8), blockWords: Array[U32](16), isRoot: Bool) <below(parentDepth, 56)>: Bool =
  val baseSlot: U64 = parentDepth * 8
  var lane: U64 = 0
  while lane < 8 do
    @<lessOrEqual(lane, 8)>
    if baseSlot < 504 then
      val leftSlot: U64 = baseSlot + lane
      if leftSlot < 512 then blockWords(lane) := stackWords(leftSlot) else ()
    else ()
    val rightLane: U64 = lane + 8
    if rightLane < 16 then blockWords(rightLane) := chain(lane) else ()
    lane = lane + 1
  var resetIdx: U64 = 0
  while resetIdx < 8 do
    @<lessOrEqual(resetIdx, 8)>
    chain(resetIdx) := Stigi.Hash.initWord(resetIdx)
    resetIdx = resetIdx + 1
  val parentFlags: U32 = if isRoot then U32.ofU64(12) else U32.ofU64(4)
  Stigi.Hash.compressBlock{region}(chain, blockWords, 0, U32.ofU64(64), parentFlags)
