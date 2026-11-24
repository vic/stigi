// prompt: apply the BLAKE3 message schedule permutation into a scratch word array.
def Stigi.Hash.permuteMessage {Stigi.Mut} (wordCount: U64, msg: Array[U32](wordCount), scratch: Array[U32](wordCount)) <lessOrEqual(16, wordCount)>: Unit =
  var slot: U64 = 0
  while slot < 16 do
    @<lessOrEqual(slot, 16)>
    scratch(slot) := msg(Stigi.Hash.scheduleSlot(slot))
    slot = slot + 1
  Stigi.Array.copy(wordCount, scratch, 0, 16, wordCount, msg, 0)
  ()
