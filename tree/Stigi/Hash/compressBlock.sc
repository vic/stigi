// prompt: compress one sixty-four-byte BLAKE3 block of the work message into its eight chaining words.
def Stigi.Hash.compressBlock {Stigi.Mut} (work: Stigi.Hash.Work, counter: U64, blockLen: U32, flags: U32) <lessOrEqual(blockLen, 64)>: Unit =
  val Work(chain, block, state, msg, scratch, _, _) = work
  Stigi.Array.copy(8, chain, 0, 8, 16, state, 0)
  state(8) := Stigi.Hash.initWord(0)
  state(9) := Stigi.Hash.initWord(1)
  state(10) := Stigi.Hash.initWord(2)
  state(11) := Stigi.Hash.initWord(3)
  state(12) := U32.ofU64(U64.bitwiseAnd(counter, 0xffffffff))
  state(13) := U32.ofU64(U64.bitwiseAnd(U64.shiftRight(counter, 32), 0xffffffff))
  state(14) := blockLen
  state(15) := flags
  Stigi.Array.copy(16, block, 0, 16, 16, msg, 0)
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
    if highIdx < 16 then chain(outIdx) := U32.bitwiseXor(state(outIdx), state(highIdx)) else ()
    outIdx = outIdx + 1
