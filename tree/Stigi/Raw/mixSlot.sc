// prompt: hash a sixty-four-bit key into a bounded table slot via Stigi.Raw.mixWord.
pub extern "c" def Stigi.Raw.mixSlot(seed: U64, slotCount: U64) <below(0, slotCount)>: U64 <below(result, slotCount)> =
  Stigi.Raw.mixWord(seed) % slotCount
