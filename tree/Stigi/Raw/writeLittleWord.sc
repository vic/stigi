// prompt: encode a little-endian thirty-two-bit word into a proved byte slice.
pub extern "c" def Stigi.Raw.writeLittleWord {Stigi.Mut} (cap: Stigi.ByteCap, bytes: Array[U8](cap), offset: U64, word: U32) <below(offset, cap), below(offset + 1, cap), below(offset + 2, cap), below(offset + 3, cap)>: Unit =
  bytes(offset) := U8.ofU64(U64.ofU32(U32.bitwiseAnd(word, 255)))
  bytes(offset + 1) := U8.ofU64(U64.ofU32(U32.bitwiseAnd(U32.shiftRight(word, 8), 255)))
  bytes(offset + 2) := U8.ofU64(U64.ofU32(U32.bitwiseAnd(U32.shiftRight(word, 16), 255)))
  bytes(offset + 3) := U8.ofU64(U64.ofU32(U32.bitwiseAnd(U32.shiftRight(word, 24), 255)))
  ()
