// prompt: compress one up-to-1024-byte BLAKE3 chunk into its eight chaining words.
def Stigi.Hash.compressChunk {region: Stigi.Region, Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), chunkStart: U64, chunkLen: U64, chunkIndex: U64, isSingle: Bool, chain: Array[U32](8), blockWords: Array[U32](16)) <lessOrEqual(chunkLen, 1024), lessOrEqual(chunkStart, cap)>: Bool =
  var initIdx: U64 = 0
  while initIdx < 8 do
    @<lessOrEqual(initIdx, 8)>
    chain(initIdx) := Stigi.Hash.initWord(initIdx)
    initIdx = initIdx + 1
  var chunkOff: U64 = 0
  var validChunk: Bool = true
  while chunkOff < chunkLen && validChunk do
    @<lessOrEqual(chunkOff, chunkLen)>
    val remChunk: U64 = chunkLen - chunkOff
    val takeLen: U64 = if remChunk < 64 then remChunk else 64
    val nextOff: U64 = chunkOff + takeLen
    val startFlag: U32 = if chunkOff == 0 then U32.ofU64(1) else U32.ofU64(0)
    val endFlag: U32 = if nextOff == chunkLen then U32.ofU64(2) else U32.ofU64(0)
    val rootFlag: U32 = if isSingle && nextOff == chunkLen then U32.ofU64(8) else U32.ofU64(0)
    val blockFlags: U32 = U32.bitwiseOr(U32.bitwiseOr(startFlag, endFlag), rootFlag)
    val blockStart: U64 = chunkStart + chunkOff
    if blockStart <= cap then
      if !Stigi.Hash.loadBlockWords{region}(cap, data, blockStart, takeLen, blockWords) then
        validChunk = false
      else if !Stigi.Hash.compressBlock{region}(chain, blockWords, chunkIndex, U32.ofU64(takeLen), blockFlags) then
        validChunk = false
      else ()
    else
      validChunk = false
    if nextOff <= chunkLen then chunkOff = nextOff else chunkOff = chunkLen
  validChunk
