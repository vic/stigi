// prompt: apply the BLAKE3 message schedule permutation in place using a scratch buffer.
def Stigi.Hash.permuteMessage {Stigi.Mut} (message: Array[U32](16), scratch: Array[U32](16)) <lessOrEqual(0, 1)>: Unit =
  scratch(0) := message(2)
  scratch(1) := message(6)
  scratch(2) := message(3)
  scratch(3) := message(10)
  scratch(4) := message(7)
  scratch(5) := message(0)
  scratch(6) := message(4)
  scratch(7) := message(13)
  scratch(8) := message(1)
  scratch(9) := message(11)
  scratch(10) := message(12)
  scratch(11) := message(5)
  scratch(12) := message(9)
  scratch(13) := message(14)
  scratch(14) := message(15)
  scratch(15) := message(8)
  var slot: U64 = 0
  while slot < 16 do
    @<lessOrEqual(slot, 16), decreases(16 - slot)>
    message(slot) := scratch(slot)
    slot = slot + 1
