// prompt: compress one sixty-four byte BLAKE3 block into the sixteen-word output state.
def Stigi.Hash.compressBlock {Stigi.Mut} (chainWords: Array[U32](8), message: Array[U32](16), scratch: Array[U32](16), state: Array[U32](16), counterLow: U32, counterHigh: U32, blockLen: U32, flags: U32) <lessOrEqual(0, 1)>: Unit =
  var index: U64 = 0
  while index < 8 do
    @<lessOrEqual(index, 8), decreases(8 - index)>
    state(index) := chainWords(index)
    index = index + 1
  state(8) := 0x6A09E667
  state(9) := 0xBB67AE85
  state(10) := 0x3C6EF372
  state(11) := 0xA54FF53A
  state(12) := counterLow
  state(13) := counterHigh
  state(14) := blockLen
  state(15) := flags
  Stigi.Hash.runRound(state, message)
  Stigi.Hash.permuteMessage(message, scratch)
  Stigi.Hash.runRound(state, message)
  Stigi.Hash.permuteMessage(message, scratch)
  Stigi.Hash.runRound(state, message)
  Stigi.Hash.permuteMessage(message, scratch)
  Stigi.Hash.runRound(state, message)
  Stigi.Hash.permuteMessage(message, scratch)
  Stigi.Hash.runRound(state, message)
  Stigi.Hash.permuteMessage(message, scratch)
  Stigi.Hash.runRound(state, message)
  Stigi.Hash.permuteMessage(message, scratch)
  Stigi.Hash.runRound(state, message)
  var lane: U64 = 0
  var upperLane: U64 = 8
  while lane < 8 && upperLane < 16 do
    @<lessOrEqual(lane, 8), lessOrEqual(upperLane, 16), decreases(8 - lane)>
    val mixedLow: U32 = U32.bitwiseXor(state(lane), state(upperLane))
    val mixedHigh: U32 = U32.bitwiseXor(state(upperLane), chainWords(lane))
    state(lane) := mixedLow
    state(upperLane) := mixedHigh
    lane = lane + 1
    upperLane = upperLane + 1
