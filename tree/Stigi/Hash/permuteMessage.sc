// prompt: apply the BLAKE3 message schedule permutation into a scratch word array.
def Stigi.Hash.permuteMessage {Stigi.Mut} (wordCount: U64, msg: Array[U32](wordCount), scratch: Array[U32](wordCount)) <lessOrEqual(16, wordCount)>: Unit =
  var slot: U64 = 0
  while slot < 16 do
    @<lessOrEqual(slot, 16)>
    val srcSlot: U64 = Stigi.Hash.scheduleSlot(slot)
    scratch(slot) := msg(srcSlot)
    slot = slot + 1
  var copyIdx: U64 = 0
  while copyIdx < 16 do
    @<lessOrEqual(copyIdx, 16)>
    msg(copyIdx) := scratch(copyIdx)
    copyIdx = copyIdx + 1
  ()
