// prompt: compute official BLAKE3 digest over a bounded byte slice with chunk tree chaining.
pub def Stigi.Hash.hashBytes {region: Stigi.Region, Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), length: U64) <lessOrEqual(length, cap)>: Stigi.Hash.HashOutcome =
  val Filled(chain) = Array.tryFill[U32]{region}(8, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(blockWords) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  val Filled(stackWords) = Array.tryFill[U32]{region}(512, U32.ofU64(0)) else
    case FillFailed => HashOutOfMemory
  var stackDepth: U64 = 0
  var chunkIndex: U64 = 0
  var pos: U64 = 0
  var failed: Bool = false
  if length == 0 then
    var initIdx: U64 = 0
    while initIdx < 8 do
      @<lessOrEqual(initIdx, 8)>
      chain(initIdx) := Stigi.Hash.initWord(initIdx)
      initIdx = initIdx + 1
    if !Stigi.Hash.compressBlock{region}(chain, blockWords, 0, U32.ofU64(0), U32.ofU64(11)) then
      failed = true
    else ()
  else
    while pos < length && !failed do
      @<lessOrEqual(pos, length), lessOrEqual(stackDepth, 56), lessOrEqual(chunkIndex, length)>
      val remAll: U64 = length - pos
      val chunkLen: U64 = if remAll < 1024 then remAll else 1024
      val isSingleChunk: Bool = chunkIndex == 0 && chunkLen == remAll
      val nextPos: U64 = pos + chunkLen
      if !Stigi.Hash.compressChunk{region}(cap, data, pos, chunkLen, chunkIndex, isSingleChunk, chain, blockWords) then
        failed = true
      else ()
      if nextPos <= length then pos = nextPos else pos = length
      if !isSingleChunk && !failed then
        var totalChunks: U64 = chunkIndex + 1
        while U64.bitwiseAnd(totalChunks, 1) == 0 && stackDepth > 0 && !failed do
          @<lessOrEqual(stackDepth, 56)>
          val parentDepth: U64 = stackDepth - 1
          val isFinalRoot: Bool = pos == length && parentDepth == 0
          if !Stigi.Hash.compressParent{region}(stackWords, parentDepth, chain, blockWords, isFinalRoot) then
            failed = true
          else ()
          stackDepth = parentDepth
          totalChunks = U64.shiftRight(totalChunks, 1)
        if pos < length && stackDepth < 56 && !failed then
          val pushBase: U64 = stackDepth * 8
          if pushBase < 504 then
            var pushLane: U64 = 0
            while pushLane < 8 do
              @<lessOrEqual(pushLane, 8)>
              val dstSlot: U64 = pushBase + pushLane
              if dstSlot < 512 then stackWords(dstSlot) := chain(pushLane) else ()
              pushLane = pushLane + 1
          else ()
          stackDepth = stackDepth + 1
        else ()
      else ()
      val nextChunk: U64 = chunkIndex + 1
      if nextChunk <= length then chunkIndex = nextChunk else failed = true
    while stackDepth > 0 && !failed do
      @<lessOrEqual(stackDepth, 56)>
      val popDepth: U64 = stackDepth - 1
      if !Stigi.Hash.compressParent{region}(stackWords, popDepth, chain, blockWords, popDepth == 0) then
        failed = true
      else ()
      stackDepth = popDepth
  if failed then HashOutOfMemory else Hashed(Digest(chain(0), chain(1), chain(2), chain(3), chain(4), chain(5), chain(6), chain(7)))
