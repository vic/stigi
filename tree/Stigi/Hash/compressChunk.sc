// prompt: compress one chunk of up to 1024 bytes into its eight-word chaining value.
def Stigi.Hash.compressChunk {Stigi.Mut} (capacity: U64, input: Array[U8](capacity), chunkStart: U64, chunkEnd: U64, chunkCounter: U64, isSingleChunk: Bool, chainWords: Array[U32](8), message: Array[U32](16), scratch: Array[U32](16), state: Array[U32](16)) <lessOrEqual(chunkStart, chunkEnd), lessOrEqual(chunkEnd, capacity), below(capacity, 0x100000000)>: Unit =
  var lane: U64 = 0
  while lane < 8 do
    @<lessOrEqual(lane, 8), decreases(8 - lane)>
    chainWords(lane) := Stigi.Hash.initWord(lane)
    lane = lane + 1
  val counterLow: U32 = U32.ofU64(U64.bitwiseAnd(chunkCounter, 0xffffffff))
  val counterHigh: U32 = U32.ofU64(U64.bitwiseAnd(U64.shiftRight(chunkCounter, 32), 0xffffffff))
  val chunkBytes: U64 = chunkEnd - chunkStart
  if chunkBytes == 0 then
    var zeroSlot: U64 = 0
    while zeroSlot < 16 do
      @<lessOrEqual(zeroSlot, 16), decreases(16 - zeroSlot)>
      message(zeroSlot) := 0
      zeroSlot = zeroSlot + 1
    val emptyFlags: U32 = if isSingleChunk then 11 else 3
    Stigi.Hash.compressBlock(chainWords, message, scratch, state, counterLow, counterHigh, 0, emptyFlags)
    var outLane: U64 = 0
    while outLane < 8 do
      @<lessOrEqual(outLane, 8), decreases(8 - outLane)>
      chainWords(outLane) := state(outLane)
      outLane = outLane + 1
  else
    var cursor: U64 = chunkStart
    var blockIndex: U64 = 0
    while cursor < chunkEnd && blockIndex < 16 do
      @<lessOrEqual(chunkStart, cursor), lessOrEqual(cursor, chunkEnd), lessOrEqual(blockIndex, 16), decreases(16 - blockIndex)>
      val plusBlock: U64 = cursor + 64
      val blockEnd: U64 = if plusBlock < chunkEnd then plusBlock else chunkEnd
      val takeBytes: U64 = blockEnd - cursor
      var wordSlot: U64 = 0
      var bytePos: U64 = cursor
      while wordSlot < 16 do
        @<lessOrEqual(wordSlot, 16), lessOrEqual(chunkStart, bytePos), lessOrEqual(bytePos, blockEnd), decreases(16 - wordSlot)>
        if bytePos < blockEnd then
          message(wordSlot) := Stigi.Hash.readLittleWord(capacity, input, bytePos, blockEnd)
          val plusFour: U64 = bytePos + 4
          bytePos = if plusFour < blockEnd then plusFour else blockEnd
        else
          message(wordSlot) := 0
        wordSlot = wordSlot + 1
      val startFlag: U32 = if blockIndex == 0 then 1 else 0
      val isLastBlock: Bool = blockEnd >= chunkEnd
      val endFlag: U32 = if isLastBlock then 2 else 0
      val rootFlag: U32 = if isLastBlock && isSingleChunk then 8 else 0
      val flags: U32 = U32.bitwiseOr(U32.bitwiseOr(startFlag, endFlag), rootFlag)
      val blockLenWord: U32 = U32.ofU64(U64.bitwiseAnd(takeBytes, 0xffffffff))
      Stigi.Hash.compressBlock(chainWords, message, scratch, state, counterLow, counterHigh, blockLenWord, flags)
      var copyLane: U64 = 0
      while copyLane < 8 do
        @<lessOrEqual(copyLane, 8), decreases(8 - copyLane)>
        chainWords(copyLane) := state(copyLane)
        copyLane = copyLane + 1
      cursor = blockEnd
      blockIndex = blockIndex + 1
