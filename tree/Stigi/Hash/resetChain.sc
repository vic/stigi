// prompt: set the eight chaining words to the BLAKE3 initialization vector.
def Stigi.Hash.resetChain {Stigi.Mut} (chain: Array[U32](8)): Unit =
  var lane: U64 = 0
  while lane < 8 do
    @<lessOrEqual(lane, 8)>
    chain(lane) := Stigi.Hash.initWord(lane)
    lane = lane + 1
