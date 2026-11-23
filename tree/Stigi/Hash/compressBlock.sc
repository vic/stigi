// prompt: compress one sixty-four-byte BLAKE3 block into eight chaining words.
def Stigi.Hash.compressBlock {region: Stigi.Region, Stigi.Mut} (chain: Array[U32](8), blockWords: Array[U32](16), counter: U64, blockLen: U32, flags: U32) <lessOrEqual(blockLen, 64)>: Bool =
  val Filled(state) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => false
  val Filled(msg) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => false
  val Filled(scratch) = Array.tryFill[U32]{region}(16, U32.ofU64(0)) else
    case FillFailed => false
  var idx: U64 = 0
  while idx < 8 do
    @<lessOrEqual(idx, 8)>
    state(idx) := chain(idx)
    idx = idx + 1
  state(8) := Stigi.Hash.initWord(0)
  state(9) := Stigi.Hash.initWord(1)
  state(10) := Stigi.Hash.initWord(2)
  state(11) := Stigi.Hash.initWord(3)
  state(12) := U32.ofU64(U64.bitwiseAnd(counter, 0xffffffff))
  state(13) := U32.ofU64(U64.bitwiseAnd(U64.shiftRight(counter, 32), 0xffffffff))
  state(14) := blockLen
  state(15) := flags
  var msgIdx: U64 = 0
  while msgIdx < 16 do
    @<lessOrEqual(msgIdx, 16)>
    msg(msgIdx) := blockWords(msgIdx)
    msgIdx = msgIdx + 1
  var round: U64 = 0
  while round < 7 do
    @<lessOrEqual(round, 7)>
    Stigi.Hash.runRound(16, state, msg)
    if round < 6 then Stigi.Hash.permuteMessage(16, msg, scratch) else ()
    round = round + 1
  var outIdx: U64 = 0
  while outIdx < 8 do
    @<lessOrEqual(outIdx, 8)>
    val highIdx: U64 = outIdx + 8
    if highIdx < 16 then
      chain(outIdx) := U32.bitwiseXor(state(outIdx), state(highIdx))
    else ()
    outIdx = outIdx + 1
  true
