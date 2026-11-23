// prompt: law proving the BLAKE3 message schedule permutation always maps inside sixteen lanes.
pub lemma def Stigi.Hash.Laws.scheduleRange(slot: U64) <below(slot, 16)>: <below(slot, 16)> =
  val mapped: U64 = Stigi.Hash.scheduleSlot(slot)
  @<below(mapped, 16)>
